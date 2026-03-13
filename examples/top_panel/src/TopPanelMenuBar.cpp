#include "TopPanelMenuBar.hpp"
#include "GlobalMenuMessage.hpp"
#include "TopPanelApplication.hpp"
#include <horizon/Logger.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/ApplicationLauncher.hpp>

using namespace horizon;

TopPanelMenuBar::TopPanelMenuBar(TopPanelApplication* app)
    : m_app(app)
{
    set_spacing(2);
    m_router = std::make_unique<RequestRouter>(m_message_manager);
    setup_router();
    setup_system_menu();

    // Wire up click callback
    when_menu_click.connect([this](MenuBarClickContext& ctx) {
        m_menu_daemon_visible = true;
        LOG_INFO << "MenuBar click: " << ctx.menu->title() << " at x=" << ctx.x
                 << " (Owner PID: " << m_current_owner_pid << ")";
        
        // y=32 because panel height is 32
        m_client_menu.show_menu(ctx.menu, ctx.x, 32, -1, "top_panel", m_current_owner_pid);
    });
}

void TopPanelMenuBar::handle_message(const std::string& msg)
{
    m_router->route(msg);
}

void TopPanelMenuBar::setup_router()
{
    m_router->register_handler(
        "set_global_menu",
        [this](const std::string& request_id, const nlohmann::json& request, MessageManager& mgr) -> nlohmann::json {
            int request_pid = request.value("pid", -1);
            auto menus_json = request.value("menus", nlohmann::json::array());
            bool is_empty = menus_json.empty();

            m_app->post_task([this, request, request_pid, is_empty]() {
                if (m_menu_daemon_visible) {
                    m_cached_menu_request = request;
                    m_has_cached_menu_request = true;
                    return;
                }

                if (!is_empty) {
                    if (m_clear_menu_timer_id) {
                        m_app->stop_timer(m_clear_menu_timer_id);
                        m_clear_menu_timer_id = 0;
                    }
                    m_current_owner_pid = request_pid;
                    apply_global_menu(request);
                } else {
                    if (m_current_owner_pid == -1 || request_pid == m_current_owner_pid) {
                        if (m_clear_menu_timer_id) {
                            m_app->stop_timer(m_clear_menu_timer_id);
                        }
                        m_clear_menu_timer_id = m_app->add_timer(100, [this]() {
                            apply_global_menu(nlohmann::json::object());
                            m_current_owner_pid = -1;
                            m_clear_menu_timer_id = 0;
                        });
                    }
                }
            });

            nlohmann::json response;
            response["status"] = "ok";
            response["request_id"] = request_id;
            return response;
        });

    m_router->register_handler(
        "menu_item_clicked",
        [this](const std::string& request_id, const nlohmann::json& request, MessageManager& mgr) -> nlohmann::json {
            std::string item_id = request.value("id", "");
            LOG_INFO << "[TOP PANEL] Menu item clicked: " << item_id;

            if (item_id == "run_terminal") {
                m_app->send_remote_signal(-1, "run_app", "konsole");
            } else if (item_id == "run_aboutus") {
                ApplicationLauncher::launch("aboutus");
            } else if (item_id == "run_logout") {
                m_app->send_remote_signal(-1, "logout");
            } else if (item_id == "force_quit") {
                if (m_current_owner_pid != -1) {
                    m_app->send_remote_signal(m_current_owner_pid, "kill");
                }
            } else if (item_id == "quit_app") {
                if (m_current_owner_pid != -1) {
                    LOG_INFO << "[TOP PANEL] Sending quit signal to PID " << m_current_owner_pid;
                    m_app->send_remote_signal(m_current_owner_pid, "quit");
                }
            } else if (item_id == "about_app") {
                 if (m_current_owner_pid != -1) {
                    m_app->send_remote_signal(m_current_owner_pid, "about");
                }
            }

            nlohmann::json response;
            response["status"] = "ok";
            response["request_id"] = request_id;
            return response;
        });

    m_router->register_handler(
        "menu_daemon_status",
        [this](const std::string& request_id, const nlohmann::json& request, MessageManager& mgr) -> nlohmann::json {
            m_menu_daemon_visible = request.value("visible", false);
            
            if (!m_menu_daemon_visible && m_has_cached_menu_request && !m_apply_cache_timer_id) {
                m_apply_cache_timer_id = m_app->add_timer(150, [this]() {
                    if (m_has_cached_menu_request) {
                        apply_global_menu(m_cached_menu_request);
                        m_has_cached_menu_request = false;
                    }
                    set_menu_open(false);
                    m_apply_cache_timer_id = 0;
                });
            } else {
                set_menu_open(m_menu_daemon_visible);
            }

            nlohmann::json response;
            response["status"] = "ok";
            response["request_id"] = request_id;
            return response;
        });
}

void TopPanelMenuBar::setup_system_menu()
{
    apply_global_menu(nlohmann::json::object());
}

void TopPanelMenuBar::apply_global_menu(const nlohmann::json& request)
{
    auto new_menus = GlobalMenuMessage::parse(request);
    clear_menus();

    // Always add system menu first
    add_menu(create_system_menu());

    for (auto& menu : new_menus) {
        add_menu(std::move(menu));
    }
    invalidate();
}

std::unique_ptr<Menu> TopPanelMenuBar::create_system_menu()
{
    auto menu = std::make_unique<Menu>();
    menu->set_title("");
    menu->set_icon_name("start-here-symbolic");
    if (menu->icon_name().empty() || IconThemeLookup::find_icon(menu->icon_name(), 18).empty()) {
        menu->set_icon_name("start-here");
    }

    menu->add_item("About This System", "", "run_aboutus");
    menu->add_separator();
    menu->add_item("Terminal", "", "run_terminal");
    menu->add_item("System Settings...");
    menu->add_item("App Store...");
    menu->add_separator();
    menu->add_item("Recent Items");
    menu->add_separator();
    menu->add_item("Force Quit...", "", "force_quit");
    menu->add_separator();
    menu->add_item("Sleep");
    menu->add_item("Restart...");
    menu->add_item("Shut Down...");
    menu->add_separator();
    menu->add_item("Lock Screen");
    menu->add_item("Log Out...", "", "run_logout");

    return menu;
}
