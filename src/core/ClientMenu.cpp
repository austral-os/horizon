#include <horizon/ClientMenu.hpp>
#include <horizon/MenuSeparator.hpp>
#include <iostream>
#include <unistd.h>

namespace horizon
{

    ClientMenu::ClientMenu(const std::string &socket_path) : m_client(socket_path) {}

    bool ClientMenu::show_menu(Menu *menu, int x, int y, int monitor)
    {
        nlohmann::json request;
        request["type"] = "create_menu";
        request["receiver_id"] = "horizon_menu_manager_d";
        request["sender_pid"] = getpid();
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

    bool ClientMenu::set_global_menu(const std::vector<Menu *> &menus)
    {
        nlohmann::json request;
        request["type"] = "set_global_menu";
        request["receiver_id"] = "top_panel";
        request["sender_pid"] = getpid();
        request["pid"] = getpid(); // Legacy compatibility

        nlohmann::json menu_array = nlohmann::json::array();
        for (auto *menu : menus)
        {
            if (menu)
            {
                menu_array.push_back(menu_to_json(menu));
            }
        }

        request["menus"] = menu_array;

        std::cout << "[ClientMenu] Sending global menu from PID " << getpid() << " with "
                  << menu_array.size() << " menus." << std::endl;

        IpcClient global_menu_client("/tmp/horizon_session.sock");
        bool success = global_menu_client.send(request.dump());

        if (!success)
        {
            std::cerr << "[ClientMenu] ERROR (PID " << getpid()
                      << "): Failed to send global menu to /tmp/horizon_session.sock. Error: "
                      << strerror(errno) << std::endl;
        }
        else
        {
            std::cout << "[ClientMenu] SUCCESS (PID " << getpid()
                      << "): Global menu sent successfully." << std::endl;
        }

        return success;
    }

    nlohmann::json ClientMenu::menu_to_json(Menu *menu)
    {
        nlohmann::json j;
        j["id"] = menu->title().empty() ? "menu" : menu->title();
        j["title"] = menu->title();
        j["bold"] = menu->bold();

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
        j["id"] = item->id().empty() ? item->text() : item->id();
        j["text"] = item->text();

        if (!item->shortcut().empty())
        {
            j["shortcut"] = item->shortcut();
        }

        return j;
    }

} // namespace horizon
