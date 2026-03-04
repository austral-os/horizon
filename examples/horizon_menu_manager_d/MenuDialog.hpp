#pragma once
#include "Dialog.hpp"
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <memory>
#include <nlohmann/json.hpp>

namespace horizon
{
    class MenuDialog : public Dialog
    {
    public:
        MenuDialog(const nlohmann::json &menu_json)
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
                    const std::string item_id = item_json.value("id", "");
                    const std::string text = item_json.value("text", "");
                    const std::string shortcut = item_json.value("shortcut", "");
                    const std::string icon = item_json.value("icon", "");

                    auto *item = menu->add_item(text, shortcut);
                    if (!icon.empty())
                    {
                        item->set_icon(icon);
                    }

                    // Store the ID as a property or handle it via signals
                    // For now, we just have it in the structure.

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
        std::unique_ptr<Menu> m_menu;
        std::vector<std::unique_ptr<Menu>> m_submenus; // Keep submenus alive
    };
} // namespace horizon
