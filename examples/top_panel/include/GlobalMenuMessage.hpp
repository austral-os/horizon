#pragma once
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

namespace horizon
{
    class GlobalMenuMessage
    {
    public:
        static std::vector<std::unique_ptr<Menu>> parse(const nlohmann::json &json,
                                                         const std::function<void(MenuItem *)> &on_click = nullptr)
        {
            std::vector<std::unique_ptr<Menu>> menus;

            if (json.contains("menus") && json["menus"].is_array())
            {
                for (const auto &menu_json : json["menus"])
                {
                    auto menu = std::make_unique<Menu>();
                    menu->set_title(menu_json.value("title", "Menu"));
                    menu->set_bold(menu_json.value("bold", false));
                    menu->set_position_type(FREE);

                    if (menu_json.contains("max_width"))
                    {
                        menu->set_max_width(menu_json["max_width"]);
                    }

                    build_menu_items(menu.get(), menu_json, on_click);
                    menus.push_back(std::move(menu));
                }
            }

            return menus;
        }

    private:
        static void build_menu_items(Menu *menu, const nlohmann::json &json,
                                     const std::function<void(MenuItem *)> &on_click)
        {
            if (json.contains("items") && json["items"].is_array())
            {
                for (const auto &item_json : json["items"])
                {
                    // Handle separators
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
                    if (!item_id.empty())
                    {
                        item->set_id(item_id);
                    }
                    if (!icon.empty())
                    {
                        item->set_icon(icon);
                    }

                    if (on_click)
                    {
                        item->when_click.connect([on_click, item](auto &)
                                                  { on_click(item); });
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

                        build_menu_items(submenu.get(), item_json["submenu"], on_click);

                        item->set_submenu(std::move(submenu));
                    }

                    // Disable local signal manager emission for global items.
                    // This prevents Top Panel from reacting to "quit" or other common IDs.
                    item->set_emit_signal_manager(false);
                }
            }
        }
    };
} // namespace horizon
