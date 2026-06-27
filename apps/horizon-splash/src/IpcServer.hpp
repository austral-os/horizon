#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace horizon
{
    class IpcServer
    {
    public:
        using MessageHandler = std::function<std::string(const std::string &)>;

        IpcServer(const std::string &socket_path, MessageHandler handler);
        ~IpcServer();

        void start();
        void stop();

        /**
         * @brief Sends a message to all persistent subscribers.
         */
        void broadcast(const std::string &msg);

    private:
        void listen_loop();
        void handle_client(int client_fd);

        std::string m_socket_path;
        MessageHandler m_message_handler;
        int m_server_fd = -1;
        std::atomic<bool> m_running{false};
        std::thread m_listen_thread;
        std::vector<std::thread> m_client_threads;

        std::mutex m_subscribers_mutex;
        std::vector<int> m_subscribers;
    };
} // namespace horizon
