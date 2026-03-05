#include "AppRegistry.hpp"
#include <chrono>
#include <csignal>
#include <fstream>
#include <horizon/IpcServer.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <spawn.h>
#include <sstream>
#include <sys/wait.h>
#include <thread>
#include <vector>

extern char **environ;

using namespace horizon;
using namespace app_manager;

void handle_signal(int sig)
{
    std::string sig_name = "unknown";
    bool is_fatal = false;

    switch (sig)
    {
    case SIGINT:
        sig_name = "SIGINT (Ctrl+C)";
        break;
    case SIGTERM:
        sig_name = "SIGTERM";
        break;
    case SIGPIPE:
        sig_name = "SIGPIPE (Broken Pipe) - SHOULD BE IGNORED!";
        break;
    case SIGSEGV:
        sig_name = "SIGSEGV (Segmentation Fault)";
        is_fatal = true;
        break;
    case SIGABRT:
        sig_name = "SIGABRT (Abort)";
        is_fatal = true;
        break;
    case SIGFPE:
        sig_name = "SIGFPE (Floating Point Exception)";
        is_fatal = true;
        break;
    case SIGILL:
        sig_name = "SIGILL (Illegal Instruction)";
        is_fatal = true;
        break;
    default:
        sig_name = std::to_string(sig);
        break;
    }

    std::cerr << "\n[SIGNAL] Received " << sig_name << "." << std::endl;
    if (is_fatal)
    {
        std::cerr << "[CRASH] App Manager is dying due to a fatal signal!" << std::endl;
        // Don't call exit(0) on fatal signals, let the system handle co-dump
        std::signal(sig, SIG_DFL);
        std::raise(sig);
    }
    else
    {
        std::cout << "Shutting down peacefully..." << std::endl;
        exit(0);
    }
}

std::string get_exec_from_desktop(const std::string &app_name)
{
    // Try both absolute and relative paths (relative to project root)
    std::string path =
        "/home/horacio/Desarrollo/austral-os/horizon/examples/config/apps/" + app_name + ".desktop";
    std::ifstream file(path);
    if (!file.is_open())
    {
        // Fallback or log error
        return "";
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.substr(0, 5) == "Exec=")
        {
            return line.substr(5);
        }
    }
    return "";
}

void run_app(const std::string &app_name)
{
    std::string exec_cmd = get_exec_from_desktop(app_name);
    if (exec_cmd.empty())
    {
        std::cerr << "[APP MANAGER] Could not find executable for: " << app_name << std::endl;
        return;
    }

    std::cout << "[APP MANAGER] Executing application: " << app_name << " (Cmd: " << exec_cmd << ")"
              << std::endl;

    pid_t pid;
    char *argv[] = {(char *)exec_cmd.c_str(), nullptr};

    // Note: posix_spawnp is safer than fork/exec and searches the PATH
    int status = posix_spawnp(&pid, exec_cmd.c_str(), nullptr, nullptr, argv, environ);

    if (status == 0)
    {
        std::cout << "[APP MANAGER] Successfully spawned " << app_name << " with PID " << pid
                  << std::endl;
    }
    else
    {
        std::cerr << "[APP MANAGER] Failed to spawn " << app_name << ": " << strerror(status)
                  << std::endl;
    }
}

int main(int argc, char *argv[])
{
    try
    {
        // Core safeguard: ignore SIGPIPE to prevent crashing on broken IPC sockets
        std::signal(SIGPIPE, SIG_IGN);

        // Register signal handlers for debugging
        std::vector<int> signals = {SIGINT, SIGTERM, SIGPIPE, SIGSEGV, SIGABRT, SIGFPE, SIGILL};
        for (int sig : signals)
        {
            std::signal(sig, handle_signal);
        }

        AppRegistry registry;

        std::cout << "Starting Horizon App Manager (Phase 2 Debugging)..." << std::endl;
        std::cout << "Listening for application events on /tmp/horizon_apps.sock" << std::endl;

        IpcServer server(
            "/tmp/horizon_apps.sock",
            [&registry, &server](const std::string &msg) -> std::string
            {
                std::cout << "[IPC] Received message: " << msg.substr(0, 100)
                          << (msg.length() > 100 ? "..." : "") << std::endl;
                try
                {
                    auto j = nlohmann::json::parse(msg);
                    std::string type = j.value("type", "unknown");

                    if (type == "subscribe")
                    {
                        std::cout << "[IPC] New subscription request." << std::endl;
                        return "SUBSCRIBE";
                    }

                    bool changed = false;
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

                        registry.add_app(info);
                        changed = true;

                        if (type == "app_started")
                            std::cout << "[EVENT] App Registered: " << info.name << " (" << info.id
                                      << ") [PID: " << info.pid << "]" << std::endl;
                        else
                            std::cout << "[EVENT] Window State Changed: " << info.name
                                      << " (Minimized: " << (info.is_minimized ? "YES" : "NO")
                                      << ")" << std::endl;
                    }
                    else if (type == "app_stopped")
                    {
                        std::string app_id = j.value("app_id", "unknown");
                        std::cout << "[EVENT] App Unregistering: " << app_id << "..." << std::endl;
                        registry.remove_app(app_id);
                        changed = true;
                        std::cout << "[EVENT] App Unregistered: " << app_id << std::endl;
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

                        nlohmann::json signal_msg;
                        signal_msg["type"] = "app_signal";
                        signal_msg["target_pid"] = target_pid;
                        signal_msg["signal"] = signal;
                        if (j.contains("token"))
                        {
                            signal_msg["token"] = j["token"];
                        }

                        std::cout << "[SIGNAL] Broadcasting " << signal << " to PID " << target_pid
                                  << std::endl;
                        server.broadcast(signal_msg.dump());

                        return "{\"status\": \"sent\"}";
                    }
                    else if (type == "run_app")
                    {
                        std::string app_name = j.value("app_name", "unknown");
                        run_app(app_name);
                        return "{\"status\": \"ok\", \"message\": \"Execution logged\"}";
                    }
                    else
                    {
                        std::cout << "[EVENT] Unknown message type: " << type << std::endl;
                    }

                    if (changed)
                    {
                        // Print current registry state
                        auto apps = registry.get_apps();
                        std::cout << "[DEBUG] Total apps in registry: " << apps.size() << std::endl;

                        // Broadcast to subscribers
                        nlohmann::json broadcast_msg;
                        broadcast_msg["type"] = "app_list_updated";
                        broadcast_msg["apps"] = nlohmann::json::array();
                        for (const auto &app : apps)
                        {
                            nlohmann::json app_j;
                            app_j["id"] = app.id;
                            app_j["name"] = app.name;
                            app_j["pid"] = app.pid;
                            app_j["icon"] = app.icon;
                            app_j["show_in_dock"] = app.show_in_dock;
                            app_j["show_in_system_tray"] = app.show_in_system_tray;
                            app_j["is_minimized"] = app.is_minimized;
                            broadcast_msg["apps"].push_back(app_j);
                        }

                        std::cout << "[IPC] Broadcasting registry update to all subscribers..."
                                  << std::endl;
                        server.broadcast(broadcast_msg.dump());
                        std::cout << "[IPC] Broadcast complete." << std::endl;
                    }
                }
                catch (const std::exception &e)
                {
                    std::cerr << "[IPC ERROR] Processing: " << e.what() << " | Raw: " << msg
                              << std::endl;
                }
                return "{\"status\": \"ok\"}";
            });

        server.start();

        // Keep the main thread alive and print heartbeats
        auto start_time = std::chrono::steady_clock::now();
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::steady_clock::now() - start_time)
                               .count();
            if (elapsed % 60 == 0)
            {
                std::cout << "[HEARTBEAT] Still alive... (Elapsed: " << elapsed << "s)"
                          << std::endl;
            }
        }

        server.stop();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL ERROR] Exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "[FATAL ERROR] Unknown error." << std::endl;
        return 1;
    }
}
