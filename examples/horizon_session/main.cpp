#include "HorizonSession.hpp"
#include "Logger.hpp"
#include <csignal>
#include <iostream>
#include <memory>

std::unique_ptr<HorizonSession> session;

void signal_handler(int sig)
{
    LOG_INFO << "Received signal " << sig << ", shutting down HorizonSession..." << std::endl;
    if (session)
    {
        session->stop();
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    bool with_compositor = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--with-compositor")
        {
            with_compositor = true;
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try
    {
        session = std::make_unique<HorizonSession>();

        session->init(with_compositor);
        session->start();

        // Keep the main thread alive. The IPC server usually runs its own event loop or thread.
        // Assuming HorizonSession start/IPC server handles its own event loop blocking,
        // if not we might need to wait here. For now, we will sleep to keep the process alive
        // since we didn't see a blocking call in start().
        while (session && session->is_running())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        LOG_INFO << "[HorizonSession] Session running flag is false, exiting main loop."
                 << std::endl;
        if (session)
        {
            session->stop();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
