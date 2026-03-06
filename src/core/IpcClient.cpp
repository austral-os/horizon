#include <horizon/IpcClient.hpp>

#include <chrono>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

namespace horizon
{

    IpcClient::IpcClient(const std::string &socket_path) : m_socket_path(socket_path) {}

    IpcClient::~IpcClient()
    {
        m_stop_subscription = true;
        if (m_subscription_fd != -1)
        {
            shutdown(m_subscription_fd, SHUT_RDWR);
            close(m_subscription_fd);
        }
        if (m_subscription_thread.joinable())
        {
            m_subscription_thread.join();
        }
    }

    bool IpcClient::send(const std::string &message, std::string &response)
    {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0)
        {
            std::cerr << "IpcClient: Failed to create socket." << std::endl;
            return false;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            close(sock);
            return false;
        }

        ssize_t sent = write(sock, message.c_str(), message.size());
        if (sent < 0)
        {
            std::cerr << "IpcClient: Failed to send data." << std::endl;
            close(sock);
            return false;
        }

        char buf[4096];
        ssize_t n = read(sock, buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = '\0';
            response = std::string(buf, n);
        }

        close(sock);
        return true;
    }

    bool IpcClient::send(const std::string &message)
    {
        std::string response;
        return send(message, response);
    }

    void IpcClient::subscribe(const std::string &message,
                              std::function<void(const std::string &)> callback)
    {
        m_subscription_thread = std::thread(
            [this, message, callback]()
            {
                while (!m_stop_subscription)
                {
                    m_subscription_fd = socket(AF_UNIX, SOCK_STREAM, 0);
                    if (m_subscription_fd < 0)
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        continue;
                    }

                    struct sockaddr_un addr;
                    memset(&addr, 0, sizeof(addr));
                    addr.sun_family = AF_UNIX;
                    strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);

                    if (connect(m_subscription_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
                    {
                        close(m_subscription_fd);
                        m_subscription_fd = -1;
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                        continue;
                    }

                    write(m_subscription_fd, message.c_str(), message.size());

                    std::string pending_data;
                    char buf[4096];
                    while (!m_stop_subscription)
                    {
                        ssize_t n = read(m_subscription_fd, buf, sizeof(buf) - 1);
                        if (n <= 0)
                            break;
                        buf[n] = '\0';
                        pending_data += buf;

                        size_t pos;
                        while ((pos = pending_data.find('\n')) != std::string::npos)
                        {
                            std::string single_msg = pending_data.substr(0, pos);
                            pending_data.erase(0, pos + 1);
                            if (!single_msg.empty())
                            {
                                callback(single_msg);
                            }
                        }
                    }

                    if (m_subscription_fd != -1)
                    {
                        close(m_subscription_fd);
                        m_subscription_fd = -1;
                    }

                    if (!m_stop_subscription)
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                    }
                }
            });
    }

} // namespace horizon
