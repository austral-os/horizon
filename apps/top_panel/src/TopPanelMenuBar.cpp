#include "TopPanelMenuBar.hpp"
#include <cstdlib>
#include "GlobalMenuMessage.hpp"
#include "TopPanelApplication.hpp"
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/Logger.hpp>
#include <horizon/I18n.hpp>
#include <horizon/XdgUserDirs.hpp>
#include <filesystem>
#include <fstream>
#include <cstdio>

using namespace horizon;

TopPanelMenuBar::TopPanelMenuBar(TopPanelApplication *app) : m_app(app)
{
    set_spacing(2);

    // Initial creation of the system menu
    auto system_menu = create_system_menu();
    // We add it directly to ensure it survives apply_global_menu calls
    add_menu(std::move(system_menu));

    // Also apply the fallback menu directly on startup since there are no active apps yet
    auto fallback = create_fallback_menu();
    if (fallback)
    {
        add_menu(std::move(fallback));
        m_app_menus_count++;
    }

    // Wire up click callback
    when_menu_click.connect(
        [this](MenuBarClickContext &ctx)
        {
            LOG_INFO << "[TOP PANEL] MenuBar click: " << ctx.menu->title() << " at x=" << ctx.x;
            m_app->window()->show_context_menu(ctx.menu, ctx.x, ctx.y - 1, ctx.serial);
        });
}

void TopPanelMenuBar::handle_message(const std::string &msg)
{
    try
    {
        auto request = nlohmann::json::parse(msg);
        std::string type = request.value("type", "");

        if (type == "set_global_menu")
        {
            int request_pid = request.value("pid", -1);
            auto menus_json = request.value("menus", nlohmann::json::array());
            bool is_empty = menus_json.empty();

            if (!is_empty)
            {
                if (m_clear_menu_timer_id)
                {
                    m_app->stop_timer(m_clear_menu_timer_id);
                    m_clear_menu_timer_id = 0;
                }
                m_current_owner_pid = request_pid;
                apply_global_menu(request);
            }
            else
            {
                // Only allow clearing if the requester is the current owner
                if (m_current_owner_pid != -1 && request_pid == m_current_owner_pid)
                {
                    if (m_clear_menu_timer_id)
                    {
                        m_app->stop_timer(m_clear_menu_timer_id);
                    }
                    m_clear_menu_timer_id =
                        m_app->add_timer(100,
                                         [this]()
                                         {
                                             apply_global_menu(nlohmann::json::object());
                                             m_current_owner_pid = -1;
                                             m_clear_menu_timer_id = 0;
                                         });
                }
            }
        }
    }
    catch (...)
    {
    }
}

void TopPanelMenuBar::apply_global_menu(const nlohmann::json &request)
{
    auto on_item_click = [this](MenuItem *item)
    {
        std::string item_id = item->id();
        if (item_id.empty())
            item_id = item->text();

        if (m_current_owner_pid != -1)
        {
            m_app->send_remote_signal(m_current_owner_pid, "menu_item_clicked", item_id);
        }
    };

    auto new_menus = GlobalMenuMessage::parse(request, on_item_click);

    if (new_menus.empty())
    {
        auto fallback = create_fallback_menu();
        if (fallback)
        {
            new_menus.push_back(std::move(fallback));
        }
    }

    // Instead of clear_menus() which clears everything, we only remove app menus.
    // Index 0 is the system menu, so we remove all menus from index 1 onwards.
    while (m_app_menus_count > 0)
    {
        remove_menu(1);
        m_app_menus_count--;
    }

    // Add new ones
    for (auto &menu : new_menus)
    {
        add_menu(std::move(menu));
        m_app_menus_count++;
    }

    invalidate();
}

static std::string urldecode(const std::string& str) {
    std::string ret;
    int ii;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            if (sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii) == 1) {
                ret += static_cast<char>(ii);
                i += 2;
            } else {
                ret += str[i];
            }
        } else {
            ret += str[i];
        }
    }
    return ret;
}

std::unique_ptr<Menu> TopPanelMenuBar::create_fallback_menu()
{
    auto main_menu = std::make_unique<Menu>();
    main_menu->set_title("Lugares");

    auto add_place = [&](const std::string& name, const std::string& icon, const std::string& path) {
        auto item = std::make_unique<MenuItem>(name);
        item->set_icon(icon);
        item->when_click.connect([path](EventContext&) {
            std::system(("arkfm \"" + path + "\" &").c_str());
        });
        main_menu->add_item(std::move(item));
    };

    add_place("Inicio", "user-home", std::getenv("HOME") ? std::string(std::getenv("HOME")) : "/");
    add_place("Escritorio", "user-desktop", XdgUserDirs::get_desktop());
    add_place("Documentos", "folder-documents", XdgUserDirs::get_documents());
    add_place("Descargas", "folder-downloads", XdgUserDirs::get_download());
    add_place("Música", "folder-music", XdgUserDirs::get_music());
    add_place("Imágenes", "folder-pictures", XdgUserDirs::get_pictures());
    add_place("Vídeos", "folder-videos", XdgUserDirs::get_videos());

    const char* home = std::getenv("HOME");
    if (home)
    {
        std::string bookmarks_path = std::string(home) + "/.config/gtk-3.0/bookmarks";
        if (std::filesystem::exists(bookmarks_path))
        {
            std::ifstream file(bookmarks_path);
            std::string line;
            bool added_separator = false;

            while (std::getline(file, line))
            {
                if (line.empty()) continue;
                
                size_t space_pos = line.find(' ');
                std::string uri = line;
                std::string name;

                if (space_pos != std::string::npos)
                {
                    uri = line.substr(0, space_pos);
                    name = line.substr(space_pos + 1);
                }
                
                if (uri.find("file://") == 0)
                {
                    std::string path = urldecode(uri.substr(7));
                    if (name.empty())
                    {
                        name = std::filesystem::path(path).filename().string();
                    }

                    if (!added_separator) {
                        main_menu->add_separator();
                        added_separator = true;
                    }

                    add_place(name, "folder-bookmark", path);
                }
            }
        }
    }
    return main_menu;
}

std::unique_ptr<Menu> TopPanelMenuBar::create_system_menu()
{
    auto menu = std::make_unique<Menu>();
    menu->set_title("");
    menu->set_icon_theme_color_key("window_fg");
    menu->set_icon_name("start-here-symbolic");
    if (menu->icon_name().empty() || IconThemeLookup::find_icon(menu->icon_name(), 18).empty())
    {
        menu->set_icon_name("start-here");
    }

    auto add_sys_item = [&](const std::string &text, const std::string &id = "")
    {
        auto *item = menu->add_item(text);
        if (!id.empty())
            item->set_id(id);
        item->set_emit_signal_manager(false);

        item->when_click.connect(
            [this, id, text](auto &)
            {
                std::string item_id = id.empty() ? text : id;
                if (item_id == "run_terminal")
                    ApplicationLauncher::launch("terminal");
                else if (item_id == "run_aboutus")
                    ApplicationLauncher::launch("aboutus");
                else if (item_id == "run_settings")
                    ApplicationLauncher::launch("preferences");
                else if (item_id == "run_logout")
                    m_app->send_remote_signal(-1, "logout");
                else if (item_id == "run_reboot")
                    std::system("systemctl reboot");
                else if (item_id == "run_poweroff")
                    std::system("systemctl poweroff");
                else if (item_id == "force_quit" && m_current_owner_pid != -1)
                    m_app->send_remote_signal(m_current_owner_pid, "kill");
            });
    };

    add_sys_item(i18n().tr("top_panel.system_menu.about"), "run_aboutus");
    menu->add_separator();
    add_sys_item(i18n().tr("top_panel.system_menu.terminal"), "run_terminal");
    add_sys_item(i18n().tr("top_panel.system_menu.settings"), "run_settings");
    menu->add_separator();
    add_sys_item(i18n().tr("top_panel.system_menu.force_quit"), "force_quit");
    menu->add_separator();
    add_sys_item(i18n().tr("top_panel.system_menu.logout"), "run_logout");
    add_sys_item(i18n().tr("top_panel.system_menu.reboot"), "run_reboot");
    add_sys_item(i18n().tr("top_panel.system_menu.poweroff"), "run_poweroff");

    return menu;
}
