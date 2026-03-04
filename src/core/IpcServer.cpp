#include <cstring>
#include <horizon/IpcServer.hpp>
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

    void IpcServer::broadcast(const std::string &msg)
    {
        std::lock_guard<std::mutex> lock(m_subscribers_mutex);
        for (auto it = m_subscribers.begin(); it != m_subscribers.end();)
        {
            ssize_t n = write(*it, msg.c_str(), msg.length());
            if (n == -1)
            {
                // Subscriber likely disconnected
                close(*it);
                it = m_subscribers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void IpcServer::handle_client(int client_fd)
    {
        char buffer[4096];
        while (m_running)
        {
            ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
            if (n <= 0)
                break;

            buffer[n] = '\0';
            std::string request(buffer);
            std::string response = m_message_handler(request);

            if (response == "SUBSCRIBE")
            {
                // Add to subscribers and STOP handling standard request/response loop here
                // but keep the socket open for broadcasts.
                {
                    std::lock_guard<std::mutex> lock(m_subscribers_mutex);
                    m_subscribers.push_back(client_fd);
                }
                // Send an initial OK so the client knows it's subscribed
                std::string ok = "{\"status\": \"subscribed\"}";
                write(client_fd, ok.c_str(), ok.length());
                return; // Return so the thread can finish, but the FD stays in m_subscribers
            }

            write(client_fd, response.c_str(), response.length());

            // If it's a one-shot request (most cases), break after first interaction
            // unless we want to support long-lived command sessions.
            // For now, let's keep it simple: one shot or subscribe.
            break;
        }
        close(client_fd);
    }
} // namespace horizon
