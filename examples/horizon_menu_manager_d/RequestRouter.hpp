#pragma once
#include "DialogManager.hpp"
#include "MenuDialog.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace horizon
{
    class RequestRouter
    {
    public:
        RequestRouter(DialogManager &dialog_manager) : m_dialog_manager(dialog_manager) {}

        nlohmann::json route(const std::string &raw_json)
        {
            try
            {
                auto request = nlohmann::json::parse(raw_json);
                std::string type = request.value("type", "");
                std::string request_id = request.value("request_id", "unknown");

                if (type == "create_menu")
                {
                    return handle_create_menu(request_id, request);
                }
                else
                {
                    return error_response(request_id, "Unknown request type: " + type);
                }
            }
            catch (const std::exception &e)
            {
                return error_response("unknown", std::string("JSON parse error: ") + e.what());
            }
        }

    private:
        nlohmann::json handle_create_menu(const std::string &request_id,
                                          const nlohmann::json &request)
        {
            if (!request.contains("menu") || !request["menu"].is_object())
            {
                return error_response(request_id, "Missing 'menu' object in create_menu request");
            }

            auto menu_dialog = std::make_unique<MenuDialog>(request["menu"]);
            std::string dialog_id = menu_dialog->id();

            m_dialog_manager.add_dialog(std::move(menu_dialog));

            nlohmann::json response;
            response["status"] = "ok";
            response["request_id"] = request_id;
            response["dialog_id"] = dialog_id;
            return response;
        }

        nlohmann::json error_response(const std::string &request_id, const std::string &message)
        {
            nlohmann::json response;
            response["status"] = "error";
            response["request_id"] = request_id;
            response["message"] = message;
            return response;
        }

        DialogManager &m_dialog_manager;
    };
} // namespace horizon
