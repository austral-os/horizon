#include "SocketListener.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <vector>

namespace horizon::secrets
{
    SocketListener::SocketListener(int uid, std::function<void(const std::string&)> on_password_received)
        : m_uid(uid), m_on_password_received(on_password_received)
    {
        m_socket_path = "/run/user/" + std::to_string(uid) + "/horizon-keyring.socket";
    }

    SocketListener::~SocketListener()
    {
        stop();
    }

    void SocketListener::start()
    {
        m_running = true;
        m_thread = std::thread(&SocketListener::listen_loop, this);
    }

    void SocketListener::stop()
    {
        m_running = false;
        if (m_server_fd != -1) {
            close(m_server_fd);
            m_server_fd = -1;
        }
        if (m_thread.joinable()) m_thread.join();
        unlink(m_socket_path.c_str());
    }

    void SocketListener::listen_loop()
    {
        m_server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_server_fd == -1) return;

        unlink(m_socket_path.c_str());

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);

        if (bind(m_server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            close(m_server_fd);
            m_server_fd = -1;
            return;
        }

        if (listen(m_server_fd, 1) == -1) {
            close(m_server_fd);
            m_server_fd = -1;
            return;
        }

        while (m_running) {
            int client_fd = accept(m_server_fd, nullptr, nullptr);
            if (client_fd == -1) {
                if (m_running) continue;
                else break;
            }

            char buffer[512];
            ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                m_on_password_received(std::string(buffer));
            }
            close(client_fd);
        }
    }
}
