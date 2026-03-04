#include <horizon/ClientMenu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/MenuSeparator.hpp>

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace horizon
{

    ClientMenu::ClientMenu(const std::string &socket_path) : m_socket_path(socket_path) {}

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

        return send_request(request);
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
            // Check if it's a separator
            if (dynamic_cast<MenuSeparator *>(child.get()))
            {
                items.push_back({{"type", "separator"}});
                continue;
            }

            // Check if it's a MenuItem
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

        // Note: submenu serialization would require access to the submenu Menu*.
        // For now, we handle flat menus. Submenus can be added later.

        return j;
    }

    bool ClientMenu::send_request(const nlohmann::json &request)
    {
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0)
        {
            std::cerr << "ClientMenu: Failed to create socket." << std::endl;
            return false;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            std::cerr << "ClientMenu: Failed to connect to " << m_socket_path << std::endl;
            close(sock);
            return false;
        }

        std::string data = request.dump();
        ssize_t sent = write(sock, data.c_str(), data.size());
        if (sent < 0)
        {
            std::cerr << "ClientMenu: Failed to send data." << std::endl;
            close(sock);
            return false;
        }

        // Read response (optional)
        char buf[4096];
        ssize_t n = read(sock, buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = '\0';
            // Could parse response here if needed
        }

        close(sock);
        return true;
    }

} // namespace horizon
