#include "AppRegistry.hpp"
#include <csignal>
#include <horizon/IpcServer.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace horizon;
using namespace app_manager;

void handle_signal(int sig)
{
    std::cout << "\nShutting down App Manager..." << std::endl;
    exit(0);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    AppRegistry registry;

    std::cout << "Starting Horizon App Manager..." << std::endl;
    std::cout << "Listening for application events on /tmp/horizon_apps.sock" << std::endl;

    IpcServer server("/tmp/horizon_apps.sock",
                     [&registry, &server](const std::string &msg) -> std::string
                     {
                         try
                         {
                             auto j = nlohmann::json::parse(msg);
                             std::string type = j.value("type", "unknown");

                             if (type == "subscribe")
                             {
                                 return "SUBSCRIBE";
                             }

                             bool changed = false;
                             if (type == "app_started")
                             {
                                 AppInfo info;
                                 info.id = j.value("app_id", "unknown");
                                 info.name = j.value("name", "Unknown");
                                 info.pid = j.value("pid", -1);
                                 info.icon = j.value("icon", "");
                                 info.show_in_dock = j.value("show_in_dock", false);
                                 info.show_in_system_tray = j.value("show_in_system_tray", false);

                                 registry.add_app(info);
                                 changed = true;

                                 std::cout << "[EVENT] App Registered: " << info.name << " ("
                                           << info.id << ") [PID: " << info.pid << "]" << std::endl;
                             }
                             else if (type == "app_stopped")
                             {
                                 std::string app_id = j.value("app_id", "unknown");
                                 registry.remove_app(app_id);
                                 changed = true;
                                 std::cout << "[EVENT] App Unregistered: " << app_id << std::endl;
                             }
                             else
                             {
                                 std::cout << "[EVENT] Unknown message type: " << type << std::endl;
                             }

                             if (changed)
                             {
                                 // Print current registry state
                                 auto apps = registry.get_apps();
                                 std::cout << "--- Registered Apps (" << apps.size() << ") ---"
                                           << std::endl;
                                 for (const auto &app : apps)
                                 {
                                     std::cout << "  - " << app.name << " (" << app.id
                                               << ") [PID: " << app.pid << "]" << std::endl;
                                 }
                                 std::cout << "-------------------------------" << std::endl;

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
                                     broadcast_msg["apps"].push_back(app_j);
                                 }
                                 server.broadcast(broadcast_msg.dump());
                             }
                         }
                         catch (const std::exception &e)
                         {
                             std::cerr << "Error parsing message: " << e.what() << " | Raw: " << msg
                                       << std::endl;
                         }
                         return "{\"status\": \"ok\"}";
                     });

    server.start();

    // Keep the main thread alive
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    return 0;
}
