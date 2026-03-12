#pragma once

#include <horizon/MenuBar.hpp>
#include <horizon/MessageManager.hpp>
#include <horizon/RequestRouter.hpp>
#include <horizon/ClientMenu.hpp>
#include <horizon/Menu.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

class TopPanelApplication;

class TopPanelMenuBar : public horizon::MenuBar
{
public:
    TopPanelMenuBar(TopPanelApplication* app);
    virtual ~TopPanelMenuBar() = default;

    void handle_message(const std::string& msg);

private:
    void setup_router();
    void setup_system_menu();
    void apply_global_menu(const nlohmann::json& request);
    std::unique_ptr<horizon::Menu> create_system_menu();

    TopPanelApplication* m_app;
    horizon::MessageManager m_message_manager;
    std::unique_ptr<horizon::RequestRouter> m_router;
    horizon::ClientMenu m_client_menu;

    // Global Menu State
    bool m_menu_daemon_visible = false;
    bool m_has_cached_menu_request = false;
    nlohmann::json m_cached_menu_request;
    size_t m_apply_cache_timer_id = 0;
    int m_current_owner_pid = -1;
    size_t m_clear_menu_timer_id = 0;
};
