#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

namespace horizon::greeter
{
    /**
     * @enum GreetdMessageType
     * @brief Types of messages from greetd to the greeter.
     */
    enum class GreetdMessageType
    {
        AuthMessage,
        Reset,
        Success,
        Error
    };

    /**
     * @struct GreetdAuthMessage
     * @brief Contains data for an authentication message from greetd.
     */
    struct GreetdAuthMessage
    {
        std::string type; // "visible", "secret", "info", "error"
        std::string message;
    };

    /**
     * @class GreetdClient
     * @brief Handles communication with the greetd daemon over the JSON protocol.
     */
    class GreetdClient
    {
    public:
        GreetdClient();
        ~GreetdClient();

        bool connect();
        void disconnect();
        bool is_connected() const { return m_fd != -1; }

        // Requests to greetd
        void create_session(const std::string &username);
        void cancel_session();
        void post_auth_message_response(const std::string &response);
        void start_session(const std::vector<std::string> &cmd, const std::vector<std::string> &env = {});
        void cancel();

        // Callbacks
        std::function<void(const GreetdAuthMessage &)> on_auth_message;
        std::function<void()> on_reset;
        std::function<void()> on_success;
        std::function<void(const std::string &, const std::string &)> on_error;

    private:
        void listen_loop();
        void send_json(const nlohmann::json &j);
        void handle_message(const nlohmann::json &j);

        int m_fd{-1};
        std::thread m_listen_thread;
        bool m_running{false};
    };
} // namespace horizon::greeter
