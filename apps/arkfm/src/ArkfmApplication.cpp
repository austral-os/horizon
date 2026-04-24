#include "ArkfmApplication.hpp"
#include "ArkfmWindow.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/Menu.hpp"
#include "horizon/I18n.hpp"

namespace horizon::arkfm
{

    const int ARK_APP_DEFAULT_WIDTH = 1000;
    const int ARK_APP_DEFAULT_HEIGHT = 700;

    ArkfmApplication::ArkfmApplication()
        : Application("org.horizon.arkfm", ARK_APP_DEFAULT_WIDTH, ARK_APP_DEFAULT_HEIGHT)
    {
        // Load translations
        i18n().load_app_locales("arkfm");

        set_name(i18n().tr("arkfm.title"));
        set_icon_name("system-file-manager");

        // Setup About info
        auto &about = about_manager();
        about.set_app_title("ArkFM");
        about.set_app_description("ArkFM is the default file manager for Horizon.");
        about.set_app_version("0.1.0");
        about.set_app_icon("system-file-manager");

        auto window = std::make_unique<ArkfmWindow>(ARK_APP_DEFAULT_WIDTH, ARK_APP_DEFAULT_HEIGHT);
        set_root(std::move(window));

        auto m_mnu_file = std::make_unique<horizon::Menu>();
        m_mnu_file->set_title(i18n().tr("arkfm.menu.file"));
        m_mnu_file->set_id("file");
        m_mnu_file->add_item(i18n().tr("arkfm.menu.open"), "Enter", "open");
        m_mnu_file->add_separator();
        m_mnu_file->add_item(i18n().tr("arkfm.menu.new_folder"), "Ctrl+Shift+N", "new-folder");
        m_mnu_file->add_separator();
        m_mnu_file->add_item(i18n().tr("arkfm.menu.delete"), "Delete", "delete");
        m_mnu_file->add_item(i18n().tr("arkfm.menu.properties"), "Alt+Return", "properties");

        auto m_mnu_edit = std::make_unique<horizon::Menu>();
        m_mnu_edit->set_title(i18n().tr("arkfm.menu.edit"));
        m_mnu_edit->set_id("edit");

        auto mnu_help = std::make_unique<horizon::Menu>();
        mnu_help->set_title(i18n().tr("arkfm.menu.help"));
        mnu_help->set_id("help");
        mnu_help->add_item(i18n().tr("arkfm.menu.about"), "F1", "aboutus"); // changed to aboutus for core signal

        add_menu(std::move(m_mnu_file));
        add_menu(std::move(m_mnu_edit));
        add_menu(std::move(mnu_help));
    }

} // namespace horizon::arkfm