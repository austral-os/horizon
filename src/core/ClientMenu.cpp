#include <horizon/ClientMenu.hpp>
#include <horizon/MenuSeparator.hpp>

namespace horizon
{

    ClientMenu::ClientMenu(const std::string &socket_path) : m_client(socket_path) {}

    bool ClientMenu::show_menu(Menu *menu, int x, int y, int monitor)
    {
        nlohmann::json request;
        request["type"] = "create_menu";
        request["request_id"] = "client_menu_" + std::to_string(reinterpret_cast<uintptr_t>(menu));
        request["x"] = x;
        request["y"] = y;

        if (monitor >= 0)
        {
            request["monitor"] = monitor;
        }

        request["menu"] = menu_to_json(menu);

        return m_client.send(request.dump());
    }

    nlohmann::json ClientMenu::menu_to_json(Menu *menu)
    {
        nlohmann::json j;
        j["id"] = menu->title().empty() ? "menu" : menu->title();
        j["title"] = menu->title();

        if (menu->max_width() > 0)
        {
            j["max_width"] = menu->max_width();
        }

        nlohmann::json items = nlohmann::json::array();

        for (const auto &child : menu->children())
        {
            if (dynamic_cast<MenuSeparator *>(child.get()))
            {
                items.push_back({{"type", "separator"}});
                continue;
            }

            auto *item = dynamic_cast<MenuItem *>(child.get());
            if (item)
            {
                items.push_back(menu_item_to_json(item));
            }
        }

        j["items"] = items;
        return j;
    }

    nlohmann::json ClientMenu::menu_item_to_json(MenuItem *item)
    {
        nlohmann::json j;
        j["id"] = item->text();
        j["text"] = item->text();

        if (!item->shortcut().empty())
        {
            j["shortcut"] = item->shortcut();
        }

        return j;
    }

} // namespace horizon
