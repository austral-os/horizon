#include "DockApplets.hpp"
#include "DockApplication.hpp"
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/Logger.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Menu.hpp>
#include <horizon/Vault.hpp>
#include <horizon/CairoGraphicsContext.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/files/FileIconProvider.hpp>
#include <horizon/arkutils/FileInfo.hpp>
#include <filesystem>
#include <cmath>

namespace horizon {

DockApplet::DockApplet(DockApplication* app, const std::string& name, const std::string& icon_name)
    : m_app(app), m_name(name)
{
    set_icon_name(icon_name);
}

// --- TrashApplet ---
TrashApplet::TrashApplet(DockApplication* app) : DockApplet(app, "Trash", "user-trash")
{
    update_icon();
    
    // Check trash status periodically
    m_timer_id = m_app->window()->add_timer(5000, [this]() {
        update_icon();
    }, true);

    when_click.connect([this](auto&) {
        std::string trash_path = "trash:///";
        ApplicationLauncher::launch_binary("arkfm", {trash_path});
    });

    when_drop.connect([this](auto& ctx) {
        std::string trash_path = std::string(getenv("HOME")) + "/.local/share/Trash/files";
        std::filesystem::create_directories(trash_path);
        auto uri_list = ctx.get_data_as_string("text/uri-list");
        LOG_INFO << "[TrashApplet] Drop received: " << uri_list;
    });

    when_right_click.connect([this](auto& ctx) {
        // Show empty trash menu
        auto menu = std::make_unique<Menu>();
        menu->set_title("trash_context");
        auto* item = menu->add_item("Empty Trash");
        item->when_click.connect([this](auto&) {
            std::string trash_path = std::string(getenv("HOME")) + "/.local/share/Trash/files";
            std::string info_path = std::string(getenv("HOME")) + "/.local/share/Trash/info";
            try {
                std::filesystem::remove_all(trash_path);
                std::filesystem::create_directories(trash_path);
                std::filesystem::remove_all(info_path);
                std::filesystem::create_directories(info_path);
            } catch(...) {}
            update_icon();
        });
        m_app->window()->show_context_menu(menu.get(), ctx.x, ctx.y, ctx.serial, this);
        m_app->window()->add_menu(std::move(menu));
    });
}

TrashApplet::~TrashApplet()
{
    if (m_timer_id) m_app->window()->stop_timer(m_timer_id);
}

void TrashApplet::update_icon()
{
    std::string trash_path = std::string(getenv("HOME")) + "/.local/share/Trash/files";
    bool empty = true;
    if (std::filesystem::exists(trash_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(trash_path)) {
            empty = false;
            break;
        }
    }
    set_icon_name(empty ? "user-trash" : "user-trash-full");
}



} // namespace horizon
