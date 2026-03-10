#include "ArkfmSidebar.hpp"

namespace horizon::arkfm
{

    ArkfmSidebar::ArkfmSidebar() : Sidebar()
    {
        add_group("Favorites");

        add_item("Favorites", std::make_unique<horizon::SidebarItem>("user-home", "All My Files"));
        add_item("Favorites",
                 std::make_unique<horizon::SidebarItem>("folder-remote", "iCloud Drive"));
        add_item("Favorites", std::make_unique<horizon::SidebarItem>("system-run", "Aplicaciones"));
        add_item("Favorites", std::make_unique<horizon::SidebarItem>("user-desktop", "Desktop"));
        add_item("Favorites",
                 std::make_unique<horizon::SidebarItem>("folder-documents", "Documents"));
        add_item("Favorites",
                 std::make_unique<horizon::SidebarItem>("folder-download", "Downloads"));
    }

} // namespace horizon::arkfm