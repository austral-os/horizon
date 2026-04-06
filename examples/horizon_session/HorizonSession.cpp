#include "HorizonSession.hpp"
#include "DesktopParser.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <horizon/Logger.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <horizon/DisplayConfig.hpp>

namespace fs = std::filesystem;

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

void HorizonSession::init(const std::string &compositor)
{
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
            m_startup_services.push_back(compositor);
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
    m_startup_services.push_back(
        "/home/horacio/Desarrollo/austral-os/horizon/build/examples/horizon_wall/horizon_wall");
    m_startup_services.push_back(
        "/home/horacio/Desarrollo/austral-os/horizon/build/examples/top_panel/top_panel");
    m_startup_services.push_back(
        "/home/horacio/Desarrollo/austral-os/horizon/build/examples/dock/dock");
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
}

void HorizonSession::stop()
{
    m_running = false;
    m_server->stop();
    terminate_all_apps();
}

void HorizonSession::terminate_all_apps()
{
    LOG_INFO << "[HorizonSession] Terminating all applications..." << std::endl;

    std::vector<int> pids_to_kill;
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        pids_to_kill = m_spawned_pids;
        for (auto const &[pid, info] : m_apps)
        {
            if (std::find(pids_to_kill.begin(), pids_to_kill.end(), pid) == pids_to_kill.end())
            {
                pids_to_kill.push_back(pid);
            }
        }
    }

    pid_t my_pid = getpid();

    for (int pid : pids_to_kill)
    {
        if (pid > 0 && pid != my_pid)
        {
            LOG_INFO << "[HorizonSession] Sending SIGTERM to PID " << pid << std::endl;
            kill(pid, SIGTERM);
        }
    }

    // Give them a moment to close
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Force SIGKILL for any stubborn survivors
    for (int pid : pids_to_kill)
    {
        if (pid > 0 && pid != my_pid)
        {
            if (kill(pid, 0) == 0) // Process still exists
            {
                LOG_INFO << "[HorizonSession] PID " << pid << " still alive, sending SIGKILL"
                         << std::endl;
                kill(pid, SIGKILL);
            }
        }
    }

    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_spawned_pids.clear();
    m_apps.clear();
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

        // Fork-safe logging (now goes to the log file because of redirection)
        LOG_INFO << "[CHILD] Spawning: " << service_path << " (PID: " << getpid() << ")";
        const char *wd = getenv("WAYLAND_DISPLAY");
        const char *path = getenv("PATH");
        LOG_INFO << "[CHILD] Environment WAYLAND_DISPLAY: " << (wd ? wd : "NULL");
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

    // Wait a moment for any previous session to fully release resources (DRM, Seat)
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (const auto &svc_path : m_startup_services)
    {
        bool is_compositor = (svc_path == "wayfire" || svc_path == "labwc");
        pid_t pid = run_service(svc_path, !is_compositor);

        // If we just started the compositor, we need to wait for its socket and set the environment
        if (is_compositor)
        {
            if (pid <= 0)
            {
                LOG_ERROR << "[HorizonSession] Failed to start compositor: " << svc_path << ". Aborting session." << std::endl;
                stop();
                return;
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
                LOG_ERROR << "[HorizonSession] Failed to detect Wayland socket. Aborting session." << std::endl;
                stop();
                return;
            }
        }
        else
        {
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

std::string HorizonSession::wait_for_new_wayland_display(const std::vector<std::string> &existing, pid_t monitor_pid)
{
    std::set<std::string> existing_set(existing.begin(), existing.end());

    for (int i = 0; i < 50; ++i) // Try for ~5 seconds
    {
        // If we are monitoring a PID, check if it's still alive
        if (monitor_pid > 0)
        {
            if (kill(monitor_pid, 0) != 0)
            {
                LOG_ERROR << "[HorizonSession] Monitored process (PID " << monitor_pid << ") terminated prematurely." << std::endl;
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
    config_path /= ".config/horizon/horizon.json";

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
                    std::string rot = (config.rotation == 90)  ? "90"
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
