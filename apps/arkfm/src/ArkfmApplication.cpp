#include "ArkfmApplication.hpp"
#include "ArkfmWindow.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/Menu.hpp"
#include "horizon/I18n.hpp"

namespace horizon::arkfm
{

    const int ARK_APP_DEFAULT_WIDTH = 1000;
    const int ARK_APP_DEFAULT_HEIGHT = 700;

    ArkfmApplication::ArkfmApplication(const std::string& initial_path)
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
        about.set_app_version(APP_VERSION);
        about.set_app_icon("system-file-manager");

        auto window = std::make_unique<ArkfmWindow>(initial_path, ARK_APP_DEFAULT_WIDTH, ARK_APP_DEFAULT_HEIGHT);
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

        auto m_mnu_view = std::make_unique<horizon::Menu>();
        m_mnu_view->set_title(i18n().tr("arkfm.menu.view"));
        m_mnu_view->set_id("view");
        m_mnu_view->add_item(i18n().tr("arkfm.menu.show_hidden"), "Ctrl+H", "toggle-hidden");

        auto m_mnu_go = std::make_unique<horizon::Menu>();
        m_mnu_go->set_title(i18n().tr("arkfm.menu.go"));
        m_mnu_go->set_id("go");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.back"), "Alt+Left", "go-back");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.forward"), "Alt+Right", "go-forward");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.parent"), "Alt+Up", "go-parent");
        m_mnu_go->add_separator();
        m_mnu_go->add_item(i18n().tr("arkfm.menu.all_files"), "Shift+Ctrl+H", "go-home");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.documents"), "Shift+Ctrl+O", "go-documents");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.desktop"), "Shift+Ctrl+D", "go-desktop");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.downloads"), "Shift+Ctrl+L", "go-downloads");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.videos"), "Shift+Ctrl+V", "go-videos");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.images"), "Shift+Ctrl+I", "go-images");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.applications"), "Shift+Ctrl+A", "go-applications");
        m_mnu_go->add_separator();
        m_mnu_go->add_item(i18n().tr("arkfm.menu.go_to_folder"), "Shift+Ctrl+G", "go-to-folder");
        m_mnu_go->add_item(i18n().tr("arkfm.menu.connect_to_server"), "Shift+Ctrl+K", "go-connect");

        auto mnu_help = std::make_unique<horizon::Menu>();
        mnu_help->set_title(i18n().tr("arkfm.menu.help"));
        mnu_help->set_id("help");
        mnu_help->add_item(i18n().tr("arkfm.menu.about"), "F1", "aboutus"); // changed to aboutus for core signal

        add_menu(std::move(m_mnu_file));
        add_menu(std::move(m_mnu_edit));
        add_menu(std::move(m_mnu_view));
        add_menu(std::move(m_mnu_go));
        add_menu(std::move(mnu_help));
    }

} // namespace horizon::arkfm