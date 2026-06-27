#pragma once
#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include "IpcServer.hpp"
#include <memory>
#include <thread>
#include <chrono>

namespace horizon
{
    class SplashWindow;

    class SplashApplication : public Application
    {
    public:
        SplashApplication();
        ~SplashApplication() override;

    private:
        WaylandLayerWindow *m_window = nullptr;
        SplashWindow *m_splash_widget = nullptr;
        
        std::unique_ptr<IpcServer> m_ipc_server;
        std::chrono::time_point<std::chrono::steady_clock> m_start_time;
        std::string handle_ipc_message(const std::string &msg);
    };

} // namespace horizon
