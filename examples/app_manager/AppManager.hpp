#pragma once
#include "AppRegistry.hpp"
#include <horizon/IpcServer.hpp>
#include <memory>
#include <string>

namespace app_manager
{
    class AppManager
    {
    public:
        AppManager();
        ~AppManager();

        void init();
        void start();
        void stop();
        void run_horizon_service(const std::string &path);

    private:
        std::string handle_ipc_message(const std::string &msg);
        void run_app_legacy(const std::string &app_name);

        AppRegistry m_registry;
        std::unique_ptr<horizon::IpcServer> m_server;
    };
} // namespace app_manager
