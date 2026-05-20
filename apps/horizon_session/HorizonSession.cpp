#include "HorizonSession.hpp"
#include "DesktopParser.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <horizon-installer-utils/InstallerManager.hpp>
#include <horizon/DisplayConfig.hpp>
#include <horizon/Logger.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

namespace
{
    bool is_setup_pending()
    {
        return horizon::installer::InstallerManager::is_oobe_pending();
    }

    bool is_setup_done()
    {
        return horizon::installer::InstallerManager::is_setup_done();
    }
} // namespace

HorizonSession::HorizonSession() : m_server_socket_path("/tmp/horizon_session.sock")
{
    m_server = std::make_unique<horizon::IpcServer>(m_server_socket_path,
                                                    [this](const std::string &msg) -> std::string
                                                    { return this->handle_ipc_message(msg); });
}

HorizonSession::~HorizonSession()
{
    stop();
}

bool HorizonSession::is_dev_mode()
{
    try
    {
        // Detect if we are running from a build directory
        auto exe_path = fs::read_symlink("/proc/self/exe");
        if (exe_path.string().find("/build/") != std::string::npos)
        {
            return true;
        }
    }
    catch (...)
    {
    }

    // Fallback/Force via environment variable
    const char *dev_env = getenv("HORIZON_DEV");
    if (dev_env && std::string(dev_env) == "1")
    {
        return true;
    }

    return false;
}

void HorizonSession::init(const std::string &compositor)
{

    // Ensure GTK icon theme is set to austral
    const char *home = std::getenv("HOME");
    if (home)
    {
        std::vector<std::string> versions = {"3.0", "4.0"};
        for (const auto &v : versions)
        {
            std::string config_dir = std::string(home) + "/.config/gtk-" + v;
            std::string settings_path = config_dir + "/settings.ini";

            try
            {
                if (!fs::exists(config_dir))
                {
                    fs::create_directories(config_dir);
                }

                bool has_settings_section = false;
                bool has_icon_theme_entry = false;
                bool has_cursor_theme_entry = false;
                std::vector<std::string> lines;

                if (fs::exists(settings_path))
                {
                    std::ifstream in(settings_path);
                    std::string line;
                    while (std::getline(in, line))
                    {
                        std::string trimmed = line;
                        // Manual trim for simplicity
                        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
                        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

                        if (trimmed == "[Settings]")
                        {
                            has_settings_section = true;
                        }

                        if (trimmed.rfind("gtk-icon-theme-name", 0) == 0)
                        {
                            lines.push_back("gtk-icon-theme-name=austral");
                            has_icon_theme_entry = true;
                        }
                        else if (trimmed.rfind("gtk-cursor-theme-name", 0) == 0)
                        {
                            lines.push_back("gtk-cursor-theme-name=austral");
                            has_cursor_theme_entry = true;
                        }
                        else
                        {
                            lines.push_back(line);
                        }
                    }
                }

                if (!has_icon_theme_entry || !has_cursor_theme_entry)
                {
                    if (!has_settings_section)
                    {
                        lines.push_back("[Settings]");
                    }

                    if (!has_icon_theme_entry)
                    {
                        lines.push_back("gtk-icon-theme-name=austral");
                    }

                    if (!has_cursor_theme_entry)
                    {
                        lines.push_back("gtk-cursor-theme-name=austral");
                    }
                }

                // Write back the whole file
                LOG_INFO << "[HorizonSession] Updating GTK " << v
                         << " settings in: " << settings_path;
                std::ofstream out(settings_path);
                for (const auto &l : lines)
                {
                    out << l << "\n";
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "[HorizonSession] Error updating GTK settings: " << e.what();
            }
        }

        // --- Cursor Theme Fallback (~/.icons/default/index.theme) ---
        try
        {
            std::string icons_default_dir = std::string(home) + "/.icons/default";
            std::string icons_index_path = icons_default_dir + "/index.theme";
            if (!fs::exists(icons_default_dir))
            {
                fs::create_directories(icons_default_dir);
            }
            
            std::ofstream icons_out(icons_index_path);
            icons_out << "[Icon Theme]\nInherits=austral\n";
            LOG_INFO << "[HorizonSession] Updated cursor theme fallback in: " << icons_index_path;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "[HorizonSession] Error updating cursor fallback: " << e.what();
        }

        // Set environment variable for the session
        setenv("XCURSOR_THEME", "austral", 1);
        LOG_INFO << "[HorizonSession] Set XCURSOR_THEME to: austral";
    }

    // Logger is now automatically initialized by the base class Application
    // if this class inherits from it, or manually here if it doesn't.
    // HorizonSession doesn't seem to inherit from Application based on its ctor.
    horizon::Logger::instance().init("horizon_session");
    LOG_INFO << "[HorizonSession] Initializing...";

    const char *wayland_display = getenv("WAYLAND_DISPLAY");
    LOG_INFO << "[HorizonSession] Current WAYLAND_DISPLAY: "
             << (wayland_display ? wayland_display : "NULL");

    if (!compositor.empty())
    {
        // Solo agregamos a m_startup_services si no estamos ya en un compositor
        if (!wayland_display)
        {
            LOG_INFO << "[HorizonSession] WAYLAND_DISPLAY is NULL, adding " << compositor
                     << " to startup services.";
            // m_startup_services.push_back(compositor);
        }
        else
        {
            LOG_INFO << "[HorizonSession] Already in a Wayland session (WAYLAND_DISPLAY="
                     << wayland_display << "), skipping compositor spawn.";
        }

        if (compositor == "labwc")
        {
            setenv("XDG_CURRENT_DESKTOP", "HZN-LABWC", 1);
        }
        else if (compositor == "wayfire")
        {
            setenv("XDG_CURRENT_DESKTOP", "HZN-WAYFIRE", 1);
        }

        LOG_INFO << "[HorizonSession] Set XDG_CURRENT_DESKTOP to: "
                 << (getenv("XDG_CURRENT_DESKTOP") ? getenv("XDG_CURRENT_DESKTOP") : "NULL");
    }

    // Example of default core services
    if (is_dev_mode())
    {
        LOG_INFO << "[HorizonSession] Development mode detected, using build paths.";
        m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) +
                                     "/apps/horizon-polkit-agent/horizon-polkit-agent");
        m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) +
                                     "/apps/horizon-powerd/horizon-powerd");
        m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) +
                                     "/apps/horizon-lens/horizon-lens");
        m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) +
                                     "/apps/horizon_wall/horizon_wall");
        m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) +
                                     "/apps/top_panel/top_panel");
        m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) + "/apps/dock/dock");
        m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) +
                                     "/apps/horizon-notifications/horizon-notifications");
    }
    else
    {
        LOG_INFO << "[HorizonSession] Production mode detected, using system paths.";
        m_startup_services.push_back("horizon-polkit-agent");
        m_startup_services.push_back("horizon-powerd");
        m_startup_services.push_back("horizon-lens");
        m_startup_services.push_back("horizon_wall");
        m_startup_services.push_back("top_panel");
        m_startup_services.push_back("dock");
        m_startup_services.push_back("horizon-notifications");
    }

    // Migrate configuration files from system to user home if not present
    if (home)
    {
        std::vector<std::string> config_files = {"desktop.json", "terminal.json",
                                                 "power.json",   "text-editor.json",
                                                 "nova.json",    "capture.json",
                                                 "mouse.json",   "notifications.json",
                                                 "color-scheme.json"};

        for (const auto &config_file : config_files)
        {
            std::string user_path = std::string(home) + "/.config/horizon/" + config_file;
            std::string system_path = "/usr/share/horizon/" + config_file;

            if (is_dev_mode())
            {
                system_path =
                    std::string(HORIZON_SOURCE_DIR) + "/apps/horizon_session/data/" + config_file;
            }

            try
            {
                // If it exists but is a symlink, remove it to ensure we create a real file
                if (fs::exists(user_path) && fs::is_symlink(user_path))
                {
                    LOG_INFO << "[HorizonSession] Removing existing symlink at: " << user_path;
                    fs::remove(user_path);
                }

                if (!fs::exists(user_path))
                {
                    LOG_INFO << "[HorizonSession] Config " << config_file
                             << " not found, checking for source at: " << system_path;
                    if (fs::exists(system_path))
                    {
                        fs::create_directories(fs::path(user_path).parent_path());
                        fs::copy_file(system_path, user_path, fs::copy_options::overwrite_existing);
                        LOG_INFO << "[HorizonSession] Successfully migrated " << config_file
                                 << " to: " << user_path;
                    }
                    else
                    {
                        LOG_ERROR << "[HorizonSession] Default config " << config_file
                                  << " NOT found at: " << system_path;
                    }
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "[HorizonSession] Error during " << config_file
                          << " migration: " << e.what();
            }
        }

        // Migrate compositor configuration files if not present
        std::vector<std::pair<std::string, std::string>> compositor_configs = {
            {"rc.xml", ".config/labwc/rc.xml"}, {"wayfire.ini", ".config/wayfire.ini"}};

        for (const auto &cfg : compositor_configs)
        {
            std::string user_path = std::string(home) + "/" + cfg.second;
            std::string system_path = "/usr/share/horizon/" + cfg.first;

            if (is_dev_mode())
            {
                system_path =
                    std::string(HORIZON_SOURCE_DIR) + "/apps/horizon_session/data/" + cfg.first;
            }

            try
            {
                if (!fs::exists(user_path))
                {
                    LOG_INFO << "[HorizonSession] Compositor config " << cfg.first
                             << " not found, checking for source at: " << system_path;
                    if (fs::exists(system_path))
                    {
                        fs::create_directories(fs::path(user_path).parent_path());
                        fs::copy_file(system_path, user_path, fs::copy_options::overwrite_existing);
                        LOG_INFO << "[HorizonSession] Successfully migrated compositor config "
                                 << cfg.first << " to: " << user_path;
                    }
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "[HorizonSession] Error during " << cfg.first
                          << " migration: " << e.what();
            }
        }

        // --- Language Initialization ---
        std::string region_path = std::string(home) + "/.config/horizon/region.json";
        std::string lang_to_set = "en";

        try
        {
            if (fs::exists(region_path))
            {
                std::ifstream f(region_path);
                nlohmann::json region_data = nlohmann::json::parse(f);
                if (region_data.contains("region") && region_data["region"].contains("language"))
                {
                    lang_to_set = region_data["region"]["language"].get<std::string>();
                    LOG_INFO << "[HorizonSession] Detected system language from region.json: "
                             << lang_to_set;
                }
            }
            else
            {
                LOG_INFO << "[HorizonSession] region.json not found, using default language: "
                         << lang_to_set;
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "[HorizonSession] Error reading region.json: " << e.what()
                      << ". Falling back to: " << lang_to_set;
        }

        setenv("LANG", lang_to_set.c_str(), 1);
        LOG_INFO << "[HorizonSession] Set session $LANG to: " << lang_to_set;
    }

    // --- OOBE / Installation Orchestration ---
    if (!is_setup_done())
    {
        if (is_setup_pending())
        {
            LOG_INFO << "[HorizonSession] Detected pending setup. Entering OOBE mode.";
        }
        else
        {
            LOG_INFO << "[HorizonSession] Detected unconfigured system. Entering scratch "
                        "installation mode.";
        }

        if (is_dev_mode())
        {
            m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) +
                                         "/apps/horizon-installer/horizon-installer");
        }
        else
        {
            m_startup_services.push_back("horizon-installer");
        }
    }
    else
    {
        // System is installed, check if we should show the welcome app
        bool show_welcome = true;
        if (home)
        {
            std::string welcome_path = std::string(home) + "/.config/horizon/austral-welcome.json";
            if (fs::exists(welcome_path))
            {
                try
                {
                    std::ifstream f(welcome_path);
                    nlohmann::json data = nlohmann::json::parse(f);
                    if (data.contains("welcome") && data["welcome"].contains("show_welcome"))
                    {
                        show_welcome = data["welcome"]["show_welcome"].get<bool>();
                    }
                }
                catch (...)
                {
                    // Ignore malformed config
                }
            }
        }

        if (show_welcome)
        {
            LOG_INFO << "[HorizonSession] Launching austral-welcome...";
            if (is_dev_mode())
            {
                m_startup_services.push_back(std::string(HORIZON_BUILD_BIN_DIR) +
                                             "/apps/austral-welcome/austral-welcome");
            }
            else
            {
                m_startup_services.push_back("austral-welcome");
            }
        }
    }
}

void HorizonSession::start()
{
    LOG_INFO << "[HorizonSession] Starting IPC Server..." << std::endl;
    m_server->start();

    // Si ya estamos en un compositor, aplicamos la configuración de pantalla de inmediato
    if (getenv("WAYLAND_DISPLAY"))
    {
        LOG_INFO << "[HorizonSession] Already in a compositor, applying display configuration...";
        apply_display_config();
    }

    run_startup_services();

    m_running = true;
    m_monitoring_thread = std::thread(&HorizonSession::monitor_services_loop, this);
}

void HorizonSession::stop()
{
    m_running = false;
    m_server->stop();

    if (m_monitoring_thread.joinable())
    {
        m_monitoring_thread.join();
    }

    terminate_all_apps();
}

void HorizonSession::terminate_all_apps()
{
    LOG_INFO << "[HorizonSession] Terminating all applications..." << std::endl;

    std::vector<int> client_pids;
    int compositor_pid = -1;

    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        compositor_pid = m_compositor_pid;

        // Collect all PIDs to kill, separating the compositor
        std::set<int> all_pids;
        for (int pid : m_spawned_pids)
            all_pids.insert(pid);
        for (auto const &[pid, info] : m_apps)
            all_pids.insert(pid);

        for (int pid : all_pids)
        {
            if (pid > 0 && pid != getpid())
            {
                if (pid == compositor_pid)
                    continue;
                client_pids.push_back(pid);
            }
        }
    }

    auto wait_for_graceful_exit = [this](const std::vector<int> &pids, int timeout_secs)
    {
        if (pids.empty())
            return;

        auto start_time = std::chrono::steady_clock::now();
        std::set<int> remaining(pids.begin(), pids.end());

        while (!remaining.empty() &&
               std::chrono::steady_clock::now() - start_time < std::chrono::seconds(timeout_secs))
        {
            for (auto it = remaining.begin(); it != remaining.end();)
            {
                int status;
                pid_t res = waitpid(*it, &status, WNOHANG);
                if (res == *it || (res == -1 && errno == ECHILD))
                {
                    it = remaining.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            if (!remaining.empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // Force SIGKILL for survivors
        for (int pid : remaining)
        {
            LOG_INFO << "[HorizonSession] PID " << pid << " still alive, sending SIGKILL"
                     << std::endl;
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
    };

    // 1. Terminate clients first
    if (!client_pids.empty())
    {
        LOG_INFO << "[HorizonSession] Sending SIGTERM to " << client_pids.size()
                 << " client applications..." << std::endl;
        for (int pid : client_pids)
            kill(pid, SIGTERM);
        wait_for_graceful_exit(client_pids, 5);
    }

    // 2. Terminate compositor last
    if (compositor_pid > 0)
    {
        LOG_INFO << "[HorizonSession] Sending SIGTERM to compositor (PID " << compositor_pid
                 << ")..." << std::endl;
        kill(compositor_pid, SIGTERM);
        wait_for_graceful_exit({compositor_pid}, 5);
    }

    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_spawned_pids.clear();
    m_apps.clear();
    m_compositor_pid = -1;
}

void HorizonSession::cleanup_lingering_compositors()
{
    LOG_INFO << "[HorizonSession] Checking for lingering compositor processes..." << std::endl;
    // We try to find wayfire/labwc not belonging to our current session or just all of them owned
    // by us if we are starting a fresh session.

    // For simplicity and safety, we only do this if we are starting a fresh session
    // (WAYLAND_DISPLAY is NULL)
    if (!getenv("WAYLAND_DISPLAY"))
    {
        LOG_INFO << "[HorizonSession] Fresh session startup, cleaning up orphaned compositors "
                    "(wayfire/labwc/Xwayland)..."
                 << std::endl;
        // pkill -u $USER will only kill our own processes.
        // We use SIGKILL (-9) here to ensure they release hardware resources immediately.
        // std::system("pkill -9 -u $USER -x wayfire || true");
        // std::system("pkill -9 -u $USER -x labwc || true");
        std::system("pkill -9 -u $USER -x Xwayland || true");
        // Give logind and the kernel a full second to release the seat and DRM master
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void HorizonSession::run_app(const std::string &app_name)
{
    LOG_INFO << "[HorizonSession] Executing app: " << app_name << std::endl;

    auto entry = DesktopParser::parse(app_name);
    if (!entry)
    {
        LOG_ERROR << "[HorizonSession] Failed to parse desktop entry for: " << app_name
                  << std::endl;
        return;
    }

    run_service(entry->exec);
}

pid_t HorizonSession::run_service(const std::string &service_path, bool use_setsid)
{
    LOG_INFO << "[HorizonSession] Executing service: " << service_path << std::endl;

    pid_t pid = fork();

    if (pid == 0)
    {
        if (use_setsid)
        {
            setsid(); // Create new session and process group
        }
        else
        {
            setpgid(0, 0); // Only new process group, keep the parent's session for seat control
        }

        prctl(PR_SET_PDEATHSIG, SIGTERM);

        // Redirect stdout and stderr to the log file for child diagnostics
        int fd = open("/tmp/horizon_session.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd != -1)
        {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        // If it's a compositor, set XDG_SESSION_TYPE
        if (!use_setsid) // This is our internal flag for compositors based on run_startup_services
                         // call site
        {
            setenv("XDG_SESSION_TYPE", "wayland", 1);
        }

        // Fork-safe logging (now goes to the log file because of redirection)
        LOG_INFO << "[CHILD] Spawning: " << service_path << " (PID: " << getpid() << ")";
        const char *wd = getenv("WAYLAND_DISPLAY");
        const char *path = getenv("PATH");
        LOG_INFO << "[CHILD] Environment WAYLAND_DISPLAY: " << (wd ? wd : "NULL");
        LOG_INFO << "[CHILD] Environment XDG_SESSION_TYPE: "
                 << (getenv("XDG_SESSION_TYPE") ? getenv("XDG_SESSION_TYPE") : "NULL");
        LOG_INFO << "[CHILD] Environment PATH: " << (path ? path : "NULL");

        char *argv[] = {(char *)service_path.c_str(), nullptr};
        execvp(service_path.c_str(), argv);

        LOG_INFO << "[HorizonSession] Failed to exec " << service_path << ": " << strerror(errno);
        _exit(1);
    }
    else if (pid > 0)
    {
        LOG_INFO << "[HorizonSession] Successfully spawned " << service_path << " with PID " << pid
                 << std::endl;
        std::lock_guard<std::mutex> lock(m_state_mutex);
        m_spawned_pids.push_back(pid);
        return pid;
    }
    else
    {
        LOG_ERROR << "[HorizonSession] Failed to fork for " << service_path << ": "
                  << strerror(errno) << std::endl;
        return -1;
    }
}

void HorizonSession::run_startup_services()
{
    auto existing_displays = get_wayland_displays();

    // Diagnostic logs for session state
    const char *session_id = getenv("XDG_SESSION_ID");
    const char *vtnr = getenv("XDG_VTNR");
    LOG_INFO << "[HorizonSession] Session ID: " << (session_id ? session_id : "NULL")
             << ", VT: " << (vtnr ? vtnr : "NULL");

    // Check if session is active via loginctl (helper command)
    if (session_id)
    {
        std::string cmd = "loginctl show-session " + std::string(session_id) + " -p Active";
        FILE *pipe = popen(cmd.c_str(), "r");
        if (pipe)
        {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe) != NULL)
            {
                LOG_INFO << "[HorizonSession] logind: " << buffer;
            }
            pclose(pipe);
        }
    }

    // Pre-flight cleanup of lingering processes from previous crashed sessions
    cleanup_lingering_compositors();

    // Wait a moment for any previous session to fully release resources (DRM, Seat)
    // Increased to 2s to be more robust against logind delay
    LOG_INFO << "[HorizonSession] Waiting for stale resources to be released...";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Cleanup stale wayland sockets/locks if they exist and we are starting fresh
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir && !getenv("WAYLAND_DISPLAY"))
    {
        try
        {
            for (const auto &entry : fs::directory_iterator(runtime_dir))
            {
                std::string filename = entry.path().filename().string();
                if (filename.find("wayland-") == 0 && filename.find(".lock") != std::string::npos)
                {
                    LOG_INFO << "[HorizonSession] Cleaning up stale lock file: " << filename;
                    fs::remove(entry.path());
                }
            }
        }
        catch (...)
        {
        }
    }

    for (const auto &svc_path : m_startup_services)
    {
        bool is_compositor = (svc_path == "wayfire" || svc_path == "labwc");
        pid_t pid = run_service(svc_path, !is_compositor);

        // If we just started the compositor, we need to wait for its socket and set the environment
        if (is_compositor)
        {
            if (pid <= 0)
            {
                LOG_ERROR << "[HorizonSession] Failed to start compositor: " << svc_path
                          << ". Aborting session." << std::endl;
                stop();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_state_mutex);
                m_compositor_pid = pid;
            }

            LOG_INFO << "[HorizonSession] Compositor started, waiting for Wayland socket..."
                     << std::endl;
            std::string new_display = wait_for_new_wayland_display(existing_displays, pid);

            if (!new_display.empty())
            {
                LOG_INFO << "[HorizonSession] Detected new WAYLAND_DISPLAY: " << new_display
                         << std::endl;
                setenv("WAYLAND_DISPLAY", new_display.c_str(), 1);

                // Apply saved display configuration
                apply_display_config();
            }
            else
            {
                LOG_ERROR << "[HorizonSession] Failed to detect Wayland socket. Aborting session."
                          << std::endl;
                LOG_ERROR << "[HorizonSession] HINT: Check /tmp/horizon_session.log for compositor "
                             "errors (e.g. 'Device or resource busy')."
                          << std::endl;
                stop();
                return;
            }
        }
        else
        {
            if (pid > 0 && is_monitored_service(svc_path))
            {
                std::lock_guard<std::mutex> lock(m_monitoring_mutex);
                MonitoredService svc;
                svc.path = svc_path;
                svc.pid = pid;
                svc.restart_count = 0;
                svc.last_restart = std::chrono::steady_clock::now();
                m_monitored_services.push_back(svc);
                LOG_INFO << "[HorizonSession] Registered " << svc_path << " (PID " << pid << ") for monitoring." << std::endl;
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)); // Space out apps to reduce concurrent congestion
        }
    }
}

std::vector<std::string> HorizonSession::get_wayland_displays()
{
    std::vector<std::string> displays;
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir)
        return displays;

    try
    {
        for (const auto &entry : fs::directory_iterator(runtime_dir))
        {
            std::string filename = entry.path().filename().string();
            if (filename.find("wayland-") == 0 && filename.find(".lock") == std::string::npos)
            {
                displays.push_back(filename);
            }
        }
    }
    catch (...)
    {
    }
    return displays;
}

std::string HorizonSession::wait_for_new_wayland_display(const std::vector<std::string> &existing,
                                                         pid_t monitor_pid)
{
    std::set<std::string> existing_set(existing.begin(), existing.end());

    for (int i = 0; i < 50; ++i) // Try for ~5 seconds
    {
        // If we are monitoring a PID, check if it's still alive
        if (monitor_pid > 0)
        {
            if (kill(monitor_pid, 0) != 0)
            {
                LOG_ERROR << "[HorizonSession] Monitored process (PID " << monitor_pid
                          << ") terminated prematurely." << std::endl;
                return "";
            }
        }

        auto current = get_wayland_displays();
        for (const auto &display : current)
        {
            if (existing_set.find(display) == existing_set.end())
            {
                return display;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return "";
}

void HorizonSession::add_app(int pid, const AppInfo &app)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_apps[pid] = app;
}

void HorizonSession::remove_app(int pid)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_apps.erase(pid);

    // Remove from subscriptions
    for (auto &[event, pids] : m_event_subscribers)
    {
        auto it = std::find(pids.begin(), pids.end(), pid);
        if (it != pids.end())
        {
            pids.erase(it);
        }
    }
}

std::optional<AppInfo> HorizonSession::get_app(int pid)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto it = m_apps.find(pid);
    if (it != m_apps.end())
    {
        return it->second;
    }
    return std::nullopt;
}

void HorizonSession::send_message(const HznSessionMessage &message)
{
    nlohmann::json j;
    j["sender_pid"] = message.sender_pid;
    j["receiver_pid"] = message.receiver_pid;
    j["sender_id"] = message.sender_id;
    j["receiver_id"] = message.receiver_id;
    j["message"] = message.message;

    m_server->broadcast(j.dump()); // Broadcast or target via specific mechanisms
}

bool HorizonSession::is_subscribed(int pid, HznSessionEvent event)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto it = m_event_subscribers.find(event);
    if (it != m_event_subscribers.end())
    {
        const auto &pids = it->second;
        return std::find(pids.begin(), pids.end(), pid) != pids.end();
    }
    return false;
}

void HorizonSession::subscribe(int pid, HznSessionEvent event)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto &pids = m_event_subscribers[event];
    if (std::find(pids.begin(), pids.end(), pid) == pids.end())
    {
        pids.push_back(pid);
        // Also add to app's personal list if needed, but not strictly necessary now
    }
}

void HorizonSession::unsubscribe(int pid, HznSessionEvent event)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto it = m_event_subscribers.find(event);
    if (it != m_event_subscribers.end())
    {
        auto &pids = it->second;
        auto pid_it = std::find(pids.begin(), pids.end(), pid);
        if (pid_it != pids.end())
        {
            pids.erase(pid_it);
        }
    }
}

std::vector<AppInfo> HorizonSession::get_subscribers(HznSessionEvent event)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    std::vector<AppInfo> result;
    auto it = m_event_subscribers.find(event);
    if (it != m_event_subscribers.end())
    {
        for (int pid : it->second)
        {
            auto app_it = m_apps.find(pid);
            if (app_it != m_apps.end())
            {
                result.push_back(app_it->second);
            }
        }
    }
    return result;
}

void HorizonSession::send_to_subscribers(HznSessionEvent event, const HznSessionMessage &message)
{
    std::vector<int> target_pids;
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        auto it = m_event_subscribers.find(event);
        if (it != m_event_subscribers.end())
        {
            target_pids = it->second;
        }
    }

    for (int pid : target_pids)
    {
        HznSessionMessage target_msg = message;
        target_msg.receiver_pid = pid;
        send_message(target_msg);
    }
}

std::string HorizonSession::handle_ipc_message(const std::string &msg)
{
    LOG_INFO << "[HorizonSession] IPC received: " << msg.substr(0, 100)
             << (msg.length() > 100 ? "..." : "") << std::endl;
    try
    {
        auto j = nlohmann::json::parse(msg);
        std::string type = j.value("type", "unknown");

        if (type == "subscribe")
        {
            LOG_INFO << "[HorizonSession] New subscription request." << std::endl;
            return "SUBSCRIBE";
        }

        bool changed = false;

        // Handling app state (similar to legacy app_manager)
        if (type == "app_started" || type == "window_state_changed")
        {
            AppInfo info;
            info.id = j.value("app_id", "unknown");
            info.name = j.value("name", "Unknown");
            info.pid = j.value("pid", -1);
            info.icon = j.value("icon", "");
            info.show_in_dock = j.value("show_in_dock", false);
            info.show_in_system_tray = j.value("show_in_system_tray", false);
            info.is_minimized = j.value("is_minimized", false);

            add_app(info.pid, info);
            changed = true;

            if (type == "app_started")
                LOG_INFO << "[EVENT] App Registered: " << info.name << " (" << info.id
                         << ") [PID: " << info.pid << "]" << std::endl;
            else
                LOG_INFO << "[EVENT] Window State Changed: " << info.name
                         << " (Minimized: " << (info.is_minimized ? "YES" : "NO") << ")"
                         << std::endl;
        }
        else if (type == "app_stopped")
        {
            std::string app_id = j.value("app_id", "unknown");
            int pid = j.value("pid", -1);
            LOG_INFO << "[EVENT] App Unregistering: " << app_id << " (PID: " << pid << ")"
                     << std::endl;
            if (pid != -1)
                remove_app(pid);
            // Si el PID no venia o era -1, idealmente habria que buscar la app y sacarla
            changed = true;
            LOG_INFO << "[EVENT] App Unregistered: " << app_id << std::endl;
        }
        else if (type == "send_signal")
        {
            int target_pid = j.value("target_pid", -1);
            std::string signal = j.value("signal", "unknown");

            if (signal == "run_app")
            {
                std::string app_name = j.value("token", "unknown");
                run_app(app_name);
                return "{\"status\": \"ok\", \"message\": \"Execution logged\"}";
            }

            if (signal == "kill")
            {
                LOG_INFO << "[SIGNAL] Terminating process PID " << target_pid << " (Force Quit)"
                         << std::endl;
                if (target_pid > 0)
                {
                    kill(target_pid, SIGKILL);
                    remove_app(target_pid);
                }
                return "{\"status\": \"killed\"}";
            }

            if (signal == "logout")
            {
                LOG_INFO << "[SIGNAL] Logout requested" << std::endl;
                terminate_all_apps();
                m_running = false;
                return "{\"status\": \"logging_out\"}";
            }

            nlohmann::json signal_msg;
            signal_msg["type"] = "app_signal";
            signal_msg["target_pid"] = target_pid;
            signal_msg["signal"] = signal;
            if (j.contains("token"))
            {
                signal_msg["token"] = j["token"];
            }

            LOG_INFO << "[SIGNAL] Broadcasting " << signal << " to PID " << target_pid << std::endl;
            m_server->broadcast(signal_msg.dump());

            return "{\"status\": \"sent\"}";
        }
        else if (type == "show_menu" || type == "menu_clicked" || type == "menu_item_clicked" ||
                 type == "menu_daemon_status" || type == "create_menu")
        {
            // Ruta de mensajeria hub and spoke para menus.
            // Para `top_panel`, o `horizon_menu_manager_d`.
            // Sender y Receiver ID/PID deberían estar en el payload JSON ahora si se están rutando.

            // Si el mensaje especifica un receiver_id o receiver_pid explícito, se lo enviaremos
            // a través del broadcast para que el destinatario lo capture,
            // ya que los clientes pueden filtrar usando receiver_id/pid.
            std::string receiver_id = j.value("receiver_id", "");
            int receiver_pid = j.value("receiver_pid", -1);

            if (!receiver_id.empty() || receiver_pid != -1)
            {
                LOG_INFO << "[ROUTING] Routing message of type " << type << " to Receiver ID: '"
                         << receiver_id << "', PID: " << receiver_pid << std::endl;
                m_server->broadcast(j.dump());
                return "{\"status\": \"routed\"}";
            }
            else
            {
                // Legacy support for directly broadcasting menu commands
                m_server->broadcast(j.dump());
                return "{\"status\": \"broadcasted\"}";
            }
        }
        else if (type == "run_app")
        {
            std::string app_name = j.value("app_name", "unknown");
            run_app(app_name);
            return "{\"status\": \"ok\", \"message\": \"Execution logged\"}";
        }
        else
        {
            // By default, if a generic message has a receiver, re-broadcast it so the targeted sub
            // gets it
            std::string receiver_id = j.value("receiver_id", "");
            int receiver_pid = j.value("receiver_pid", -1);

            if (!receiver_id.empty() || receiver_pid != -1)
            {
                m_server->broadcast(j.dump());
                return "{\"status\": \"routed\"}";
            }
            LOG_INFO << "[EVENT] Unknown message type: " << type << std::endl;
        }

        if (changed)
        {
            // Print current registry state
            std::lock_guard<std::mutex> lock(m_state_mutex);
            LOG_INFO << "[DEBUG] Total apps in registry: " << m_apps.size() << std::endl;

            // Broadcast to subscribers
            nlohmann::json broadcast_msg;
            broadcast_msg["type"] = "app_list_updated";
            broadcast_msg["apps"] = nlohmann::json::array();
            for (const auto &[pid, app] : m_apps)
            {
                nlohmann::json app_j;
                app_j["app_id"] = app.id;
                app_j["name"] = app.name;
                app_j["pid"] = app.pid;
                app_j["icon"] = app.icon;
                app_j["show_in_dock"] = app.show_in_dock;
                app_j["show_in_system_tray"] = app.show_in_system_tray;
                app_j["is_minimized"] = app.is_minimized;
                broadcast_msg["apps"].push_back(app_j);
            }

            LOG_INFO << "[IPC] Broadcasting registry update to all subscribers..." << std::endl;
            m_server->broadcast(broadcast_msg.dump());
            LOG_INFO << "[IPC] Broadcast complete." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "[IPC ERROR] Processing: " << e.what() << " | Raw: " << msg << std::endl;
        return "{\"status\": \"error\"}";
    }
    return "{\"status\": \"ok\"}";
}

void HorizonSession::apply_display_config()
{
    const char *home = std::getenv("HOME");
    if (!home)
        return;

    fs::path config_path(home);
    config_path /= ".config/horizon/display.json";

    if (!fs::exists(config_path))
    {
        LOG_INFO << "[HorizonSession] No configuration file found at " << config_path;
        return;
    }

    try
    {
        std::ifstream file(config_path);
        nlohmann::json j;
        file >> j;

        if (!j.contains("displays") || !j["displays"].is_array())
        {
            LOG_INFO << "[HorizonSession] No 'displays' section found in " << config_path;
            return;
        }

        LOG_INFO << "[HorizonSession] Applying display configuration from horizon.json";

        std::vector<horizon::MonitorConfig> configs;
        for (const auto &item : j["displays"])
        {
            horizon::MonitorConfig cfg;
            cfg.name = item.value("name", "");
            cfg.x = item.value("x", 0);
            cfg.y = item.value("y", 0);
            cfg.width = item.value("width", 0);
            cfg.height = item.value("height", 0);
            cfg.refresh = item.value("refresh", 60.0f);
            cfg.rotation = item.value("rotation", 0);
            cfg.enabled = item.value("enabled", true);
            configs.push_back(cfg);
        }

        // Determine compositor and apply
        const char *desktop = std::getenv("XDG_CURRENT_DESKTOP");
        std::string desktop_str = desktop ? desktop : "";

        LOG_INFO << "[HorizonSession] Applying configuration for desktop: " << desktop_str;

        if (desktop_str.find("LABWC") != std::string::npos ||
            desktop_str.find("WAYFIRE") != std::string::npos)
        {
            // Use wlr-randr
            for (const auto &config : configs)
            {
                std::stringstream ss;
                ss << "wlr-randr --output " << config.name;
                if (config.enabled)
                {
                    ss << " --mode " << config.width << "x" << config.height;
                    ss << " --pos " << config.x << "," << config.y;
                    std::string rot = (config.rotation == 90)    ? "90"
                                      : (config.rotation == 180) ? "180"
                                      : (config.rotation == 270) ? "270"
                                                                 : "normal";
                    ss << " --transform " << rot;
                }
                else
                {
                    ss << " --off";
                }
                LOG_INFO << "[HorizonSession] Executing: " << ss.str();
                std::system(ss.str().c_str());
            }
        }
        else if (desktop_str.find("KDE") != std::string::npos)
        {
            // Use kscreen-doctor
            std::stringstream ss;
            ss << "kscreen-doctor";
            for (const auto &config : configs)
            {
                ss << " output." << config.name;
                if (config.enabled)
                {
                    ss << ".mode." << config.width << "x" << config.height << "@"
                       << (int)config.refresh;
                    ss << " output." << config.name << ".position." << config.x << "," << config.y;
                    ss << " output." << config.name << ".rotation." << config.rotation;
                }
                else
                {
                    ss << ".disable";
                }
            }
            LOG_INFO << "[HorizonSession] Executing: " << ss.str();
            std::system(ss.str().c_str());
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "[HorizonSession] Error applying display configuration: " << e.what();
    }
}

bool HorizonSession::is_monitored_service(const std::string &path)
{
    std::string bin_name = path;
    size_t last_slash = path.find_last_of('/');
    if (last_slash != std::string::npos)
    {
        bin_name = path.substr(last_slash + 1);
    }

    static const std::set<std::string> critical_services = {
        "horizon-polkit-agent",
        "horizon-powerd",
        "horizon_wall",
        "top_panel",
        "dock",
        "horizon-notifications"
    };

    return critical_services.find(bin_name) != critical_services.end();
}

void HorizonSession::monitor_services_loop()
{
    LOG_INFO << "[HorizonSession] Started monitoring background loop." << std::endl;
    while (m_running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (!m_running)
            break;

        int status;
        pid_t exited_pid;
        while ((exited_pid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            if (WIFEXITED(status))
            {
                LOG_INFO << "[HorizonSession] Child process with PID " << exited_pid 
                         << " exited with status " << WEXITSTATUS(status) << std::endl;
            }
            else if (WIFSIGNALED(status))
            {
                LOG_INFO << "[HorizonSession] Child process with PID " << exited_pid 
                         << " terminated by signal " << WTERMSIG(status) << std::endl;
            }
            else
            {
                LOG_INFO << "[HorizonSession] Child process with PID " << exited_pid 
                         << " terminated." << std::endl;
            }

            // Clean up dead processes from standard session tracking
            {
                std::lock_guard<std::mutex> state_lock(m_state_mutex);
                auto it = std::find(m_spawned_pids.begin(), m_spawned_pids.end(), exited_pid);
                if (it != m_spawned_pids.end())
                {
                    m_spawned_pids.erase(it);
                }
            }
            remove_app(exited_pid);

            // Check if this PID is one of our monitored services
            std::string service_path_to_restart;
            bool should_restart = false;
            MonitoredService* target_svc = nullptr;

            {
                std::lock_guard<std::mutex> monitor_lock(m_monitoring_mutex);
                for (auto &svc : m_monitored_services)
                {
                    if (svc.pid == exited_pid)
                    {
                        target_svc = &svc;
                        should_restart = true;
                        break;
                    }
                }

                if (should_restart && target_svc)
                {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - target_svc->last_restart).count();

                    if (elapsed > 10)
                    {
                        target_svc->restart_count = 0;
                    }

                    if (target_svc->restart_count >= 5)
                    {
                        LOG_ERROR << "[HorizonSession] Service " << target_svc->path 
                                  << " has restarted too many times recently. Disabling auto-restart." << std::endl;
                        should_restart = false;
                    }
                    else
                    {
                        target_svc->restart_count++;
                        target_svc->last_restart = now;
                        service_path_to_restart = target_svc->path;
                    }
                }
            }

            if (should_restart && !service_path_to_restart.empty() && m_running)
            {
                LOG_INFO << "[HorizonSession] Restarting critical service: " << service_path_to_restart << std::endl;

                pid_t new_pid = run_service(service_path_to_restart, true);
                if (new_pid > 0)
                {
                    std::lock_guard<std::mutex> monitor_lock(m_monitoring_mutex);
                    for (auto &svc : m_monitored_services)
                    {
                        if (svc.path == service_path_to_restart)
                        {
                            svc.pid = new_pid;
                            break;
                        }
                    }
                    LOG_INFO << "[HorizonSession] Service " << service_path_to_restart 
                             << " successfully restarted with new PID " << new_pid << std::endl;
                }
                else
                {
                    LOG_ERROR << "[HorizonSession] Failed to restart service: " << service_path_to_restart << std::endl;
                }
            }
        }
    }
    LOG_INFO << "[HorizonSession] Stopped monitoring background loop." << std::endl;
}
