#include "SocketListener.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <horizon/Logger.hpp>

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
        if (m_server_fd == -1) {
            LOG_ERROR << "[Keyring] Failed to create UDS socket";
            return;
        }

        unlink(m_socket_path.c_str());

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);

        if (bind(m_server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            LOG_ERROR << "[Keyring] Failed to bind UDS socket to " << m_socket_path;
            close(m_server_fd);
            m_server_fd = -1;
            return;
        }

        if (listen(m_server_fd, 5) == -1) {
            LOG_ERROR << "[Keyring] Failed to listen on UDS socket";
            close(m_server_fd);
            m_server_fd = -1;
            return;
        }

        LOG_INFO << "[Keyring] Listening for PAM messages on " << m_socket_path;

        while (m_running) {
            int client_fd = accept(m_server_fd, nullptr, nullptr);
            if (client_fd == -1) {
                if (m_running) continue;
                else break;
            }

            // Security: Verify peer credentials (SO_PEERCRED)
            struct ucred cr;
            socklen_t len = sizeof(struct ucred);
            if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cr, &len) == 0) {
                if (cr.uid != (uid_t)m_uid && cr.uid != 0) {
                    LOG_ERROR << "[Keyring] Security breach: Connection from unauthorized UID " << cr.uid;
                    close(client_fd);
                    continue;
                }
            }

            // Protocol: [uint32_t length][password string]
            uint32_t pass_len = 0;
            if (read(client_fd, &pass_len, sizeof(pass_len)) == sizeof(pass_len)) {
                if (pass_len > 0 && pass_len < 1024) { // Sanity check
                    std::vector<char> pass_buf(pass_len + 1);
                    ssize_t n = read(client_fd, pass_buf.data(), pass_len);
                    if (n == (ssize_t)pass_len) {
                        pass_buf[pass_len] = '\0';
                        m_on_password_received(std::string(pass_buf.data()));
                    }
                }
            }
            
            close(client_fd);
        }
    }
}
