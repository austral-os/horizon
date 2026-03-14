#include "ArkfmApplication.hpp"
#include "ArkfmWindow.hpp"
#include "horizon/Menu.hpp"

namespace horizon::arkfm
{

    const int ARK_APP_DEFAULT_WIDTH = 1000;
    const int ARK_APP_DEFAULT_HEIGHT = 700;

    ArkfmApplication::ArkfmApplication()
        : WaylandWindow("org.horizon.arkfm", ARK_APP_DEFAULT_WIDTH, ARK_APP_DEFAULT_HEIGHT)
    {
        set_name("Ark File Manager");
        auto window = std::make_unique<ArkfmWindow>(ARK_APP_DEFAULT_WIDTH, ARK_APP_DEFAULT_HEIGHT);

        set_root(std::move(window));

        auto m_mnu_file = std::make_unique<horizon::Menu>();
        m_mnu_file->set_title("Archivo");
        m_mnu_file->add_item("Nueva Carpeta", "Ctrl+Shift+N", "new-folder");

        auto mnu_help = std::make_unique<horizon::Menu>();
        mnu_help->set_title("Ayuda");
        mnu_help->add_item("Acerca de", "F1", "about");

        add_menu(std::move(m_mnu_file));
        add_menu(std::move(mnu_help));
    }

} // namespace horizon::arkfm