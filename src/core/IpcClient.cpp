#include <horizon/IpcClient.hpp>

#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace horizon
{

    IpcClient::IpcClient(const std::string &socket_path) : m_socket_path(socket_path) {}

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
            std::cerr << "IpcClient: Failed to connect to " << m_socket_path << std::endl;
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

} // namespace horizon
