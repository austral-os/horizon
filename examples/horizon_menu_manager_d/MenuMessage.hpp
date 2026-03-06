#pragma once
#include <horizon/IpcClient.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/Message.hpp>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

namespace horizon
{
    class MenuMessage : public Message
    {
    public:
        MenuMessage(const nlohmann::json &menu_json, const std::string &requester_id = "",
                    int requester_pid = -1, std::function<void()> on_action = nullptr)
            : m_requester_id(requester_id), m_requester_pid(requester_pid), m_on_action(on_action)
        {
            m_id = menu_json.value("id", "unknown");
            m_menu = std::make_unique<Menu>();
            m_menu->set_position(100, 100); // Default position, can be adjusted
            m_menu->set_position_type(FREE);
            if (menu_json.contains("max_width"))
            {
                m_menu->set_max_width(menu_json["max_width"]);
            }

            build_menu(m_menu.get(), menu_json);
        }

        const std::string &id() const override
        {
            return m_id;
        }
        Widget *root_widget() override
        {
            return m_menu.get();
        }

        void show() override
        {
            m_menu->set_visible(true);
        }
        void hide() override
        {
            m_menu->set_visible(false);
        }

        // Take ownership of everything we created
        std::vector<std::unique_ptr<Menu>> release_all_menus()
        {
            std::vector<std::unique_ptr<Menu>> all;
            if (m_menu)
                all.push_back(std::move(m_menu));
            for (auto &sm : m_submenus)
            {
                all.push_back(std::move(sm));
            }
            m_submenus.clear();
            return all;
        }

    private:
        void build_menu(Menu *menu, const nlohmann::json &json)
        {
            if (json.contains("items") && json["items"].is_array())
            {
                for (const auto &item_json : json["items"])
                {
                    if (item_json.value("type", "") == "separator")
                    {
                        menu->add_separator();
                        continue;
                    }

                    const std::string item_id = item_json.value("id", "");
                    const std::string text = item_json.value("text", "");
                    const std::string shortcut = item_json.value("shortcut", "");
                    const std::string icon = item_json.value("icon", "");

                    auto *item = menu->add_item(text, shortcut);
                    if (!icon.empty())
                    {
                        item->set_icon(icon);
                    }

                    if (!item_id.empty())
                    {
                        item->set_id(item_id);
                        std::cout << "[MENU MESSAGE] Adding click handler for item: " << item_id
                                  << std::endl;
                        item->add_on_click(
                            [item_id, this]()
                            {
                                std::cout << "[MENU MANAGER] MenuItem clicked! ID: " << item_id
                                          << ". Reporting to /tmp/horizon_global_menu.sock"
                                          << std::endl;
                                try
                                {
                                    IpcClient client("/tmp/horizon_session.sock");
                                    nlohmann::json msg;
                                    msg["type"] = "menu_item_clicked";
                                    // Route back to the requester (e.g., top_panel)
                                    if (!m_requester_id.empty())
                                    {
                                        msg["receiver_id"] = m_requester_id;
                                    }
                                    else
                                    {
                                        msg["receiver_id"] = "top_panel"; // Fallback
                                    }

                                    if (m_requester_pid != -1)
                                    {
                                        msg["receiver_pid"] = m_requester_pid;
                                    }
                                    msg["id"] = item_id;
                                    bool ok = client.send(msg.dump());
                                    std::cout << "[MENU MANAGER] Report sent: "
                                              << (ok ? "SUCCESS" : "FAILED") << std::endl;

                                    if (m_on_action)
                                    {
                                        m_on_action();
                                    }
                                }
                                catch (const std::exception &e)
                                {
                                    std::cerr
                                        << "[MENU MANAGER] ERROR reporting click: " << e.what()
                                        << std::endl;
                                }
                            });
                    }

                    if (item_json.contains("submenu") && item_json["submenu"].is_object())
                    {
                        auto submenu = std::make_unique<Menu>();
                        submenu->set_visible(false);
                        submenu->set_position_type(FREE);
                        if (item_json["submenu"].contains("max_width"))
                        {
                            submenu->set_max_width(item_json["submenu"]["max_width"]);
                        }
                        build_menu(submenu.get(), item_json["submenu"]);

                        item->set_submenu(submenu.get());
                        m_submenus.push_back(std::move(submenu));
                    }
                }
            }
        }

        std::string m_id;
        std::string m_requester_id;
        int m_requester_pid;
        std::function<void()> m_on_action;
        std::unique_ptr<Menu> m_menu;
        std::vector<std::unique_ptr<Menu>> m_submenus; // Keep submenus alive
    };
} // namespace horizon
