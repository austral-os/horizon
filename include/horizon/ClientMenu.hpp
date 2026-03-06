#pragma once

#include <horizon/IpcClient.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace horizon
{
    /**
     * @brief IPC client that communicates with horizon_menu_manager_d
     *        to display menus via Unix socket.
     */
    class ClientMenu
    {
    public:
        ClientMenu(const std::string &socket_path = "/tmp/horizon_session.sock");
        ~ClientMenu() = default;

        /**
         * @brief Sends a Menu to the menu manager daemon for display.
         * @param menu The menu to display.
         * @param x X position on screen.
         * @param y Y position on screen.
         * @param monitor Optional monitor index (-1 = default).
         * @return true if the request was sent successfully.
         */
        bool show_menu(Menu *menu, int x, int y, int monitor = -1);

        /**
         * @brief Sends a list of Menus to the global menu panel.
         * @param menus A list of root menus to display.
         * @return true if the request was sent successfully.
         */
        bool set_global_menu(const std::vector<Menu *> &menus);

    private:
        nlohmann::json menu_to_json(Menu *menu);
        nlohmann::json menu_item_to_json(MenuItem *item);

        IpcClient m_client;
    };

} // namespace horizon
