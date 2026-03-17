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
    setup_system_menu();

    // Wire up click callback to use the new direct menu system
    when_menu_click.connect([this](MenuBarClickContext& ctx) {
        LOG_INFO << "[TOP PANEL] MenuBar click: " << ctx.menu->title() << " at x=" << ctx.x
                 << " (Owner PID: " << m_current_owner_pid << ")";
        
        // y=32 because panel height is 32. Using show_context_menu directly.
        m_app->window()->show_context_menu(ctx.menu, ctx.x, 32);
    });
}

void TopPanelMenuBar::handle_message(const std::string& msg)
{
    try {
        auto request = nlohmann::json::parse(msg);
        std::string type = request.value("type", "");

        if (type == "set_global_menu") {
            int request_pid = request.value("pid", -1);
            auto menus_json = request.value("menus", nlohmann::json::array());
            bool is_empty = menus_json.empty();

            LOG_INFO << "[TOP PANEL] Received set_global_menu from PID "
                     << request_pid << " (menus count: " << menus_json.size() << ")";

            m_app->post_task([this, request, request_pid, is_empty]() {
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
        }
        else if (type == "menu_daemon_status") {
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
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "[TOP PANEL] Error parsing menu message: " << e.what();
    }
}

void TopPanelMenuBar::setup_router()
{
    // No longer used, logic moved to handle_message
}

void TopPanelMenuBar::setup_system_menu()
{
    apply_global_menu(nlohmann::json::object());
}

void TopPanelMenuBar::apply_global_menu(const nlohmann::json& request)
{
    // Handler for clicks on app-specific menu items
    auto on_item_click = [this](MenuItem* item) {
        std::string item_id = item->id();
        if (item_id.empty()) item_id = item->text();

        LOG_INFO << "[TOP PANEL] Global menu item clicked: " << item_id 
                 << " (Owner PID: " << m_current_owner_pid << ")";

        // Check if it's a system menu item we know how to handle
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
        } else if (m_current_owner_pid != -1) {
            // It's a normal menu item from another app, send the click back via IPC
            m_app->send_remote_signal(m_current_owner_pid, "menu_item_clicked", item_id);
        }
    };

    auto new_menus = GlobalMenuMessage::parse(request, on_item_click);
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

    // Since we use the same on_item_click handler in apply_global_menu, 
    // and that handler checks for IDs, we don't need to manually connect signals here
    // IF we use GlobalMenuMessage::parse or similar logic, but create_system_menu
    // manually adds items. So we DO need to connect signals here if we don't call
    // the common handler.
    
    auto add_sys_item = [&](const std::string& text, const std::string& shortcut = "", const std::string& id = "") {
        auto* item = menu->add_item(text, shortcut);
        if (!id.empty()) item->set_id(id);
        
        item->when_click.connect([this, id, text](auto&) {
            std::string item_id = id.empty() ? text : id;
            LOG_INFO << "[TOP PANEL] System menu item clicked: " << item_id;
            
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
            }
        });
    };

    add_sys_item("About This System", "", "run_aboutus");
    menu->add_separator();
    add_sys_item("Terminal", "", "run_terminal");
    add_sys_item("System Settings...");
    add_sys_item("App Store...");
    menu->add_separator();
    add_sys_item("Recent Items");
    menu->add_separator();
    add_sys_item("Force Quit...", "", "force_quit");
    menu->add_separator();
    add_sys_item("Sleep");
    add_sys_item("Restart...");
    add_sys_item("Shut Down...");
    menu->add_separator();
    add_sys_item("Lock Screen");
    add_sys_item("Log Out...", "", "run_logout");

    return menu;
}
