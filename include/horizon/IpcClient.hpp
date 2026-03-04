#pragma once

#include <string>

namespace horizon
{
    /**
     * @brief Generic IPC client for Unix socket communication.
     */
    class IpcClient
    {
    public:
        explicit IpcClient(const std::string &socket_path);
        ~IpcClient() = default;

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

    private:
        std::string m_socket_path;
    };

} // namespace horizon
