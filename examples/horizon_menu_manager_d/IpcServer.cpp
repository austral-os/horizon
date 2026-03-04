#include "IpcServer.hpp"
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace horizon
{
    IpcServer::IpcServer(const std::string &socket_path, MessageHandler handler)
        : m_socket_path(socket_path), m_message_handler(handler)
    {
    }

    IpcServer::~IpcServer()
    {
        stop();
    }

    void IpcServer::start()
    {
        if (m_running)
            return;

        m_server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_server_fd == -1)
        {
            perror("socket");
            return;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);

        unlink(m_socket_path.c_str());

        if (bind(m_server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("bind");
            close(m_server_fd);
            return;
        }

        if (listen(m_server_fd, 5) == -1)
        {
            perror("listen");
            close(m_server_fd);
            return;
        }

        m_running = true;
        m_listen_thread = std::thread(&IpcServer::listen_loop, this);
        std::cout << "IPC Server listening on " << m_socket_path << std::endl;
    }

    void IpcServer::stop()
    {
        if (!m_running)
            return;
        m_running = false;

        // Close server socket to break accept()
        if (m_server_fd != -1)
        {
            shutdown(m_server_fd, SHUT_RDWR);
            close(m_server_fd);
            m_server_fd = -1;
        }

        if (m_listen_thread.joinable())
        {
            m_listen_thread.join();
        }

        for (auto &t : m_client_threads)
        {
            if (t.joinable())
                t.join();
        }

        unlink(m_socket_path.c_str());
    }

    void IpcServer::listen_loop()
    {
        while (m_running)
        {
            int client_fd = accept(m_server_fd, nullptr, nullptr);
            if (client_fd == -1)
            {
                if (m_running)
                    perror("accept");
                continue;
            }

            m_client_threads.emplace_back(&IpcServer::handle_client, this, client_fd);
        }
    }

    void IpcServer::handle_client(int client_fd)
    {
        char buffer[4096];
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = '\0';
            std::string request(buffer);
            std::string response = m_message_handler(request);
            write(client_fd, response.c_str(), response.length());
        }
        close(client_fd);
    }
} // namespace horizon
