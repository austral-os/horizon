#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace horizon::secrets
{
    class SocketListener
    {
    public:
        SocketListener(int uid, std::function<void(const std::string&)> on_password_received);
        ~SocketListener();

        void start();
        void stop();

    private:
        int m_uid;
        std::function<void(const std::string&)> m_on_password_received;
        std::string m_socket_path;
        int m_server_fd{-1};
        std::atomic<bool> m_running{false};
        std::thread m_thread;

        void listen_loop();
    };
}
