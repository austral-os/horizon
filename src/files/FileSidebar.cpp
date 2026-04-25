#include "horizon/files/FileSidebar.hpp"
#include <horizon/XdgUserDirs.hpp>
#include <horizon/SidebarItem.hpp>
#include <cstdlib>

namespace horizon::files
{
    FileSidebar::FileSidebar() : Sidebar()
    {
        add_group("Favorites");

        auto home = getenv("HOME") ? getenv("HOME") : "/home/user";

        auto item_home = std::make_unique<horizon::SidebarItem>("user-home", "All My Files");
        item_home->set_path(home);
        add_item("Favorites", std::move(item_home));

        auto item_apps = std::make_unique<horizon::SidebarItem>("system-run", "Aplicaciones");
        item_apps->set_path("/usr/share/applications");
        add_item("Favorites", std::move(item_apps));

        auto item_desktop = std::make_unique<horizon::SidebarItem>("user-desktop", "Desktop");
        item_desktop->set_path(horizon::XdgUserDirs::get_desktop());
        add_item("Favorites", std::move(item_desktop));

        auto item_docs = std::make_unique<horizon::SidebarItem>("folder-documents", "Documents");
        item_docs->set_path(horizon::XdgUserDirs::get_documents());
        add_item("Favorites", std::move(item_docs));

        auto item_downloads = std::make_unique<horizon::SidebarItem>("folder-download", "Downloads");
        item_downloads->set_path(horizon::XdgUserDirs::get_download());
        add_item("Favorites", std::move(item_downloads));
    }
} // namespace horizon::files
