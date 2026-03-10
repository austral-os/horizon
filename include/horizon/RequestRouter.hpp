#pragma once
#include <functional>
#include <horizon/Logger.hpp>
#include <horizon/MessageManager.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace horizon
{
    /**
     * @brief Extensible JSON request router.
     *        Handlers are registered per request type string.
     */
    class RequestRouter
    {
    public:
        using HandlerFn = std::function<nlohmann::json(
            const std::string &request_id, const nlohmann::json &request, MessageManager &manager)>;

        explicit RequestRouter(MessageManager &message_manager) : m_message_manager(message_manager)
        {
        }

        void register_handler(const std::string &type, HandlerFn handler)
        {
            m_handlers[type] = std::move(handler);
        }

        nlohmann::json route(const std::string &raw_json)
        {
            LOG_INFO << "[RequestRouter] Routing message: " << raw_json;
            try
            {
                auto request = nlohmann::json::parse(raw_json);
                std::string type = request.value("type", "");
                std::string request_id = request.value("request_id", "unknown");

                auto it = m_handlers.find(type);
                if (it != m_handlers.end())
                {
                    LOG_INFO << "[RequestRouter] Found handler for type: " << type;
                    return it->second(request_id, request, m_message_manager);
                }
                else
                {
                    LOG_ERROR << "[RequestRouter] No handler for type: " << type;
                    return error_response(request_id, "Unknown request type: " + type);
                }
            }
            catch (const std::exception &e)
            {
                return error_response("unknown", std::string("JSON parse error: ") + e.what());
            }
        }

    private:
        static nlohmann::json error_response(const std::string &request_id,
                                             const std::string &message)
        {
            nlohmann::json response;
            response["status"] = "error";
            response["request_id"] = request_id;
            response["message"] = message;
            return response;
        }

        MessageManager &m_message_manager;
        std::unordered_map<std::string, HandlerFn> m_handlers;
    };
} // namespace horizon
