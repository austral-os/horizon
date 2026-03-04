#include <csignal>
#include <horizon/IpcServer.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace horizon;

void handle_signal(int sig)
{
    std::cout << "\nShutting down App Manager..." << std::endl;
    exit(0);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::cout << "Starting Horizon App Manager..." << std::endl;
    std::cout << "Listening for application events on /tmp/horizon_apps.sock" << std::endl;

    IpcServer server("/tmp/horizon_apps.sock",
                     [](const std::string &msg) -> std::string
                     {
                         try
                         {
                             auto j = nlohmann::json::parse(msg);
                             std::string type = j.value("type", "unknown");
                             std::string name = j.value("name", "Unknown");
                             std::string app_id = j.value("app_id", "unknown");
                             int pid = j.value("pid", -1);

                             if (type == "app_started")
                             {
                                 std::cout << "[EVENT] App Started: " << name << " (" << app_id
                                           << ") [PID: " << pid << "]" << std::endl;
                                 std::cout << "        Icon: " << j.value("icon", "") << std::endl;
                                 std::cout << "        Dock: "
                                           << (j.value("show_in_dock", false) ? "Yes" : "No")
                                           << std::endl;
                                 std::cout << "        Tray: "
                                           << (j.value("show_in_system_tray", false) ? "Yes" : "No")
                                           << std::endl;
                             }
                             else if (type == "app_stopped")
                             {
                                 std::cout << "[EVENT] App Stopped: " << app_id << " [PID: " << pid
                                           << "]" << std::endl;
                             }
                             else
                             {
                                 std::cout << "[EVENT] Unknown: " << msg << std::endl;
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
