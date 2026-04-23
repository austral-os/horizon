#include "GreetdClient.hpp"
#include <fcntl.h>
#include <horizon/Logger.hpp>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace horizon::greeter
{
    GreetdClient::GreetdClient() = default;

    GreetdClient::~GreetdClient()
    {
        disconnect();
    }

    bool GreetdClient::connect()
    {
        const char *sock_path = getenv("GREETD_SOCK");
        if (!sock_path)
        {
            LOG_ERROR << "GreetdClient: GREETD_SOCK environment variable not set.";
            // Test if FD 3 is valid (common in some greetd environments)
            if (fcntl(3, F_GETFD) != -1)
            {
                LOG_INFO << "GreetdClient: Using file descriptor 3.";
                m_fd = 3;
            }
            else
            {
                return false;
            }
        }
        else
        {
            m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (m_fd < 0)
            {
                LOG_ERROR << "GreetdClient: Failed to create socket.";
                return false;
            }

            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

            if (::connect(m_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                LOG_ERROR << "GreetdClient: Failed to connect to greetd socket.";
                close(m_fd);
                m_fd = -1;
                return false;
            }
        }

        m_running = true;
        m_listen_thread = std::thread(&GreetdClient::listen_loop, this);
        return true;
    }

    void GreetdClient::disconnect()
    {
        m_running = false;
        if (m_fd != -1)
        {
            shutdown(m_fd, SHUT_RDWR);
            close(m_fd);
            m_fd = -1;
        }
        if (m_listen_thread.joinable())
        {
            m_listen_thread.join();
        }
    }

    void GreetdClient::create_session(const std::string &username)
    {
        nlohmann::json j;
        j["type"] = "create_session";
        j["username"] = username;
        send_json(j);
    }

    void GreetdClient::post_auth_message_response(const std::string &response)
    {
        nlohmann::json j;
        j["type"] = "post_auth_message_response";
        j["response"] = response;
        send_json(j);
    }

    void GreetdClient::start_session(const std::vector<std::string> &cmd, const std::vector<std::string> &env)
    {
        nlohmann::json j;
        j["type"] = "start_session";
        j["cmd"] = cmd;
        if (!env.empty())
        {
            j["env"] = env;
        }
        send_json(j);
    }

    void GreetdClient::cancel()
    {
        nlohmann::json j;
        j["type"] = "cancel";
        send_json(j);
    }

    void GreetdClient::send_json(const nlohmann::json &j)
    {
        if (m_fd == -1)
            return;
        std::string s = j.dump();
        uint32_t len = static_cast<uint32_t>(s.size());
        
        // Greetd protocol: 4-byte little-endian length prefix
        write(m_fd, &len, 4);
        write(m_fd, s.c_str(), s.size());
    }

    void GreetdClient::listen_loop()
    {
        while (m_running)
        {
            uint32_t len = 0;
            // 1. Read the 4-byte length prefix
            ssize_t n = read(m_fd, &len, 4);
            if (n <= 0) 
            {
                if (m_running)
                {
                    LOG_ERROR << "GreetdClient: Lost connection to greetd while reading length.";
                }
                break;
            }
            if (n != 4) 
            {
                LOG_ERROR << "GreetdClient: Incomplete length read: " << n << " bytes.";
                continue;
            }

            // 2. Read the JSON message of 'len' bytes
            std::vector<char> buffer(len + 1);
            ssize_t total_read = 0;
            while (total_read < (ssize_t)len) 
            {
                n = read(m_fd, buffer.data() + total_read, len - total_read);
                if (n <= 0) break;
                total_read += n;
            }
            
            if (total_read < (ssize_t)len) 
            {
                LOG_ERROR << "GreetdClient: Incomplete message read: " << total_read << "/" << len;
                break;
            }
            
            buffer[len] = '\0';

            try
            {
                auto j_obj = nlohmann::json::parse(buffer.data());
                handle_message(j_obj);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR << "GreetdClient: JSON parse error: " << e.what() << " content: " << buffer.data();
            }
        }
        
        m_running = false;
    }

    void GreetdClient::handle_message(const nlohmann::json &j)
    {
        std::string type = j.value("type", "");
        if (type == "auth_message")
        {
            if (on_auth_message)
            {
                GreetdAuthMessage msg;
                msg.type = j.value("auth_message_type", "");
                msg.message = j.value("message", "");
                on_auth_message(msg);
            }
        }
        else if (type == "reset")
        {
            if (on_reset)
                on_reset();
        }
        else if (type == "success")
        {
            if (on_success)
                on_success();
        }
        else if (type == "error")
        {
            if (on_error)
            {
                on_error(j.value("error_type", ""), j.value("description", ""));
            }
        }
    }
} // namespace horizon::greeter
