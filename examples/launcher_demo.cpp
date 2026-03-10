#include <horizon/ApplicationLauncher.hpp>
#include <horizon/DesktopEntry.hpp>
#include <horizon/Logger.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

using namespace horizon;

int main(int argc, char **argv)
{
    std::cout << "ApplicationLauncher Demo" << std::endl;
    std::cout << "My PID: " << getpid() << std::endl;

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <app_name_or_id_or_path>" << std::endl;
        std::cout << "Example: " << argv[0] << " xclock (binary in path)" << std::endl;
        std::cout << "Example: " << argv[0] << " firefox (binary or desktop)" << std::endl;
        std::cout << "Example: " << argv[0] << " /usr/bin/xclock (direct path)" << std::endl;
        return 1;
    }

    // Test custom search path if an extra argument is provided
    if (argc >= 3)
    {
        std::string custom_path = argv[2];
        std::cout << "Adding custom search path: " << custom_path << std::endl;
        DesktopEntry::add_search_path(custom_path);
    }

    std::string target = argv[1];

    bool success = false;
    if (target.find("/") != std::string::npos)
    {
        std::cout << "Attempting to launch as direct binary path: " << target << std::endl;
        success = ApplicationLauncher::launch_binary(target);
    }
    else
    {
        std::cout << "Attempting to launch via smart 'launch' method: " << target << std::endl;
        success = ApplicationLauncher::launch(target);
    }

    if (success)
    {
        std::cout << "Launch sequence successful. Check if the app is running." << std::endl;
        std::cout << "This parent process will exit in 3 seconds. The app should remain running."
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    else
    {
        std::cerr << "Launch failed!" << std::endl;
        return 1;
    }

    return 0;
}
