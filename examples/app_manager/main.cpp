#include "AppManager.hpp"
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

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
        std::signal(sig, SIG_DFL);
        std::raise(sig);
    }
    else
    {
        std::cout << "Shutting down peacefully..." << std::endl;
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    try
    {
        std::signal(SIGPIPE, SIG_IGN);

        std::vector<int> signals = {SIGINT, SIGTERM, SIGPIPE, SIGSEGV, SIGABRT, SIGFPE, SIGILL};
        for (int sig : signals)
        {
            std::signal(sig, handle_signal);
        }

        AppManager manager;

        std::cout << "Starting Horizon App Manager..." << std::endl;
        std::cout << "Listening for application events on /tmp/horizon_apps.sock" << std::endl;

        manager.start();

        // Give the IPC server a moment to start up before initializing services
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        manager.init();

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

        manager.stop();
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
