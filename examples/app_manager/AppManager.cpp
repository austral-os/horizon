#include "AppManager.hpp"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <thread>

extern char **environ;

namespace app_manager
{

    AppManager::AppManager()
    {
        m_server = std::make_unique<horizon::IpcServer>(
            "/tmp/horizon_apps.sock", [this](const std::string &msg) -> std::string
            { return this->handle_ipc_message(msg); });
    }

    AppManager::~AppManager()
    {
        stop();
    }

    void AppManager::run_horizon_service(const std::string &path)
    {
        std::cout << "[APP MANAGER] Executing service: " << path << std::endl;

        pid_t pid = fork();

        if (pid == 0)
        {
            prctl(PR_SET_PDEATHSIG, SIGTERM);
            char *argv[] = {(char *)path.c_str(), nullptr};
            execvp(path.c_str(), argv);

            std::cerr << "[APP MANAGER] Failed to exec " << path << ": " << strerror(errno)
                      << std::endl;
            exit(1);
        }
        else if (pid > 0)
        {
            std::cout << "[APP MANAGER] Successfully spawned " << path << " with PID " << pid
                      << std::endl;
        }
        else
        {
            std::cerr << "[APP MANAGER] Failed to fork for " << path << ": " << strerror(errno)
                      << std::endl;
        }
    }

    void AppManager::run_app_legacy(const std::string &app_name)
    {
        // Try both absolute and relative paths (relative to project root)
        std::string path = "/home/horacio/Desarrollo/austral-os/horizon/examples/config/apps/" +
                           app_name + ".desktop";
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "[APP MANAGER] Could not find executable desktop for: " << app_name
                      << std::endl;
            return;
        }

        std::string exec_cmd = "";
        std::string line;
        while (std::getline(file, line))
        {
            if (line.substr(0, 5) == "Exec=")
            {
                exec_cmd = line.substr(5);
                break;
            }
        }

        if (exec_cmd.empty())
        {
            std::cerr << "[APP MANAGER] Could not find Exec in desktop file for: " << app_name
                      << std::endl;
            return;
        }

        std::cout << "[APP MANAGER] Executing application: " << app_name << " (Cmd: " << exec_cmd
                  << ")" << std::endl;

        pid_t pid = fork();

        if (pid == 0)
        {
            prctl(PR_SET_PDEATHSIG, SIGTERM);
            char *argv[] = {(char *)exec_cmd.c_str(), nullptr};
            execvp(exec_cmd.c_str(), argv);

            std::cerr << "[APP MANAGER] Failed to exec " << app_name << ": " << strerror(errno)
                      << std::endl;
            exit(1);
        }
        else if (pid > 0)
        {
            std::cout << "[APP MANAGER] Successfully spawned " << app_name << " with PID " << pid
                      << std::endl;
        }
        else
        {
            std::cerr << "[APP MANAGER] Failed to fork for " << app_name << ": " << strerror(errno)
                      << std::endl;
        }
    }

    void AppManager::init(bool with_compositor)
    {
        std::cout << "[APP MANAGER] Initializing core services..." << std::endl;

        struct ServiceConfig
        {
            std::string name;
            std::string path;
            int timeout_ms;
        };

        std::vector<ServiceConfig> services;

        if (with_compositor)
        {
            services.push_back({"labwc", "labwc", 5000});
        }

        services.push_back({"horizon_wall",
                            "/home/horacio/Desarrollo/austral-os/horizon/build/horizon_wall",
                            5000});
        services.push_back(
            {"horizon_menu_manager_d",
             "/home/horacio/Desarrollo/austral-os/horizon/build/horizon_menu_manager_d", 5000});
        services.push_back(
            {"top_panel", "/home/horacio/Desarrollo/austral-os/horizon/build/top_panel", 5000});
        services.push_back(
            {"dock", "/home/horacio/Desarrollo/austral-os/horizon/build/dock", 5000});

        for (const auto &svc : services)
        {
            std::cout << "[APP MANAGER] Starting " << svc.name << "..." << std::endl;
            run_horizon_service(svc.path);

            std::cout << "[APP MANAGER] Waiting for " << svc.name << " to register..." << std::endl;
            if (m_registry.wait_for_app(svc.name, svc.timeout_ms))
            {
                std::cout << "[APP MANAGER] " << svc.name << " registered successfully."
                          << std::endl;
            }
            else
            {
                std::cout << "[APP MANAGER] WARNING: " << svc.name
                          << " failed to register within timeout." << std::endl;
            }
        }
        std::cout << "[APP MANAGER] Initialization complete." << std::endl;
    }

    void AppManager::start()
    {
        m_server->start();
    }

    void AppManager::stop()
    {
        m_server->stop();
    }

    std::string AppManager::handle_ipc_message(const std::string &msg)
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

                m_registry.add_app(info);
                changed = true;

                if (type == "app_started")
                    std::cout << "[EVENT] App Registered: " << info.name << " (" << info.id
                              << ") [PID: " << info.pid << "]" << std::endl;
                else
                    std::cout << "[EVENT] Window State Changed: " << info.name
                              << " (Minimized: " << (info.is_minimized ? "YES" : "NO") << ")"
                              << std::endl;
            }
            else if (type == "app_stopped")
            {
                std::string app_id = j.value("app_id", "unknown");
                std::cout << "[EVENT] App Unregistering: " << app_id << "..." << std::endl;
                m_registry.remove_app(app_id);
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
                    run_app_legacy(app_name);
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
                m_server->broadcast(signal_msg.dump());

                return "{\"status\": \"sent\"}";
            }
            else if (type == "run_app")
            {
                std::string app_name = j.value("app_name", "unknown");
                run_app_legacy(app_name);
                return "{\"status\": \"ok\", \"message\": \"Execution logged\"}";
            }
            else
            {
                std::cout << "[EVENT] Unknown message type: " << type << std::endl;
            }

            if (changed)
            {
                // Print current registry state
                auto apps = m_registry.get_apps();
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
                m_server->broadcast(broadcast_msg.dump());
                std::cout << "[IPC] Broadcast complete." << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[IPC ERROR] Processing: " << e.what() << " | Raw: " << msg << std::endl;
        }
        return "{\"status\": \"ok\"}";
    }

} // namespace app_manager
