#include <cstring>
#include <horizon/IpcServer.hpp>
#include <iostream>
#include <signal.h>
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

        // Ignore SIGPIPE to prevent the process from crashing if a client disconnects during write
        signal(SIGPIPE, SIG_IGN);

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

        if (listen(m_server_fd, 32) == -1)
        {
            perror("listen");
            close(m_server_fd);
            return;
        }

        m_running = true;
        m_listen_thread = std::thread(&IpcServer::listen_loop, this);
        std::cout << "[IpcServer] PID " << getpid() << " listening on " << m_socket_path
                  << " (fd: " << m_server_fd << ")" << std::endl;
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

        // Detached threads don't need to be joined here.
        // If we really wanted to wait for them, we'd need a counter and a condition variable.
        // For now, detaching is enough to avoid the leak of thread objects.

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
                    perror("[IpcServer] accept failed");
                continue;
            }

            std::cout << "[IpcServer] New connection accepted on fd: " << client_fd << std::endl;
            std::thread(&IpcServer::handle_client, this, client_fd).detach();
        }
    }

    void IpcServer::broadcast(const std::string &msg)
    {
        std::string payload = msg;
        if (payload.empty() || payload.back() != '\n')
        {
            payload += '\n';
        }
        std::lock_guard<std::mutex> lock(m_subscribers_mutex);
        for (auto it = m_subscribers.begin(); it != m_subscribers.end();)
        {
            // Use send with MSG_NOSIGNAL to prevent SIGPIPE if the subscriber disconnected
            ssize_t n = send(*it, payload.c_str(), payload.length(), MSG_NOSIGNAL);
            if (n == -1)
            {
                std::cout << "[IpcServer] Subscriber on fd " << *it
                          << " disconnected during broadcast." << std::endl;
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
        std::cout << "[IpcServer] Client connected (fd: " << client_fd << ")" << std::endl;
        char buffer[4096];
        while (m_running)
        {
            ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
            if (n <= 0)
                break;

            buffer[n] = '\0';
            std::string request(buffer);
            std::cout << "[IpcServer] Read " << n << " bytes from client." << std::endl;
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
                std::string ok = "{\"status\": \"subscribed\"}\n";
                send(client_fd, ok.c_str(), ok.length(), MSG_NOSIGNAL);
                return; // Return so the thread can finish, but the FD stays in m_subscribers
            }

            send(client_fd, response.c_str(), response.length(), MSG_NOSIGNAL);

            // If it's a one-shot request (most cases), break after first interaction
            // unless we want to support long-lived command sessions.
            // For now, let's keep it simple: one shot or subscribe.
            break;
        }
        close(client_fd);
    }
} // namespace horizon
