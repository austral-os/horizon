#pragma once
#include <functional>
#include <string>
#include <thread>

namespace horizon
{
    /**
     * @brief Generic IPC client for Unix socket communication.
     */
    class IpcClient
    {
    public:
        explicit IpcClient(const std::string &socket_path);
        ~IpcClient();

        /**
         * @brief Sends a message and receives a response.
         * @param message The message string to send.
         * @param response Output parameter for the response.
         * @return true if send/receive succeeded.
         */
        bool send(const std::string &message, std::string &response);

        /**
         * @brief Sends a message without waiting for a response.
         */
        bool send(const std::string &message);

        /**
         * @brief Subscribes to messages from the server.
         * @param message The message to send to initiate subscription (e.g., "{"type":
         * "subscribe"}").
         * @param callback Callback to invoke for each message received from the server.
         */
        void subscribe(const std::string &message,
                       std::function<void(const std::string &)> callback);

    private:
        std::string m_socket_path;
        std::thread m_subscription_thread;
        bool m_stop_subscription = false;
        int m_subscription_fd = -1;
    };

} // namespace horizon
