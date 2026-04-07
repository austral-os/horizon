#include "HorizonSession.hpp"
#include <csignal>
#include <horizon/Logger.hpp>
#include <memory>

std::unique_ptr<HorizonSession> session;

void signal_handler(int sig)
{
    LOG_INFO << "Received signal " << sig << ", shutting down HorizonSession...";
    if (session)
    {
        session->stop();
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    std::string compositor = "";
    // Parse command line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --help, -h               Show this help message" << std::endl;
            std::cout << "  --compositor <name>      Specify the compositor to start (e.g., labwc, "
                         "wayfire)"
                      << std::endl;
            std::cout << "  --with-compositor        Alias for --compositor labwc" << std::endl;
            return 0;
        }
        else if (arg == "--with-compositor")
        {
            compositor = "labwc";
        }
        else if (arg == "--compositor" && i + 1 < argc)
        {
            compositor = argv[++i];
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try
    {
        session = std::make_unique<HorizonSession>();

        session->init(compositor);
        session->start();

        // Keep the main thread alive. The IPC server usually runs its own event loop or thread.
        // Assuming HorizonSession start/IPC server handles its own event loop blocking,
        // if not we might need to wait here. For now, we will sleep to keep the process alive
        // since we didn't see a blocking call in start().
        while (session && session->is_running())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        LOG_INFO << "[HorizonSession] Session running flag is false, exiting main loop.";
        if (session)
        {
            session->stop();
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Fatal error: " << e.what();
        return 1;
    }

    return 0;
}
