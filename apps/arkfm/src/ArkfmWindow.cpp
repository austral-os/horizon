#include "ArkfmWindow.hpp"
#include "dialogs/ConnectToServerDialog.hpp"
#include "dialogs/GoToFolderDialog.hpp"
#include "dialogs/NewFolderDialog.hpp"
#include "dialogs/PropertiesDialog.hpp"
#include "dialogs/RenameDialog.hpp"
#include "horizon/ApplicationLauncher.hpp"
#include "horizon/ApplicationWindow.hpp"
#include "horizon/DesktopManager.hpp"
#include "horizon/Label.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Menu.hpp"
#include "horizon/ProgressBar.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/VPanel.hpp"
#include "horizon/Widget.hpp"
#include "horizon/arkutils/FileOperations.hpp"
#include "horizon/compression/CompressionManager.hpp"
#include "horizon/dialogs/AppPickerDialog.hpp"
#include <algorithm>
#include <filesystem>
#include <horizon/DialogTypes.hpp>
#include <horizon/I18n.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/NotificationSender.hpp>
#include <horizon/files/FileIconProvider.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/files/FileSidebar.hpp>
#include <horizon/files/FileToolbar.hpp>
#include <horizon/files/FileView.hpp>
#include <horizon/storage/MountPasswordDialog.hpp>
#include <horizon/storage/RemoteManager.hpp>
#include <memory>
#include <thread>
#include <fstream>
#include <sstream>

namespace horizon::arkfm
{

    ArkfmWindow::ArkfmWindow(const std::string& initial_path, int w, int h) : ApplicationWindow("Ark File Manager")
    {
        set_size(w, h);
        auto ark_toolbar = std::make_unique<files::FileToolbar>();
        auto *ark_toolbar_ptr = ark_toolbar.get();
        toolbar()->add_toolbar_widget(std::move(ark_toolbar));
        show_status_bar();

        auto vpanel = std::make_unique<horizon::VPanel>();
        vpanel->set_spacing(10);

        auto sidebar = std::make_unique<files::FileSidebar>();
        m_sidebar_ptr = sidebar.get();
        auto *sidebar_ptr = m_sidebar_ptr;
        std::string start_path = initial_path.empty() ? (getenv("HOME") ? getenv("HOME") : "~/") : initial_path;
        auto view = std::make_unique<files::FileView>(start_path);
        m_view_ptr = view.get();
        auto *view_ptr = m_view_ptr;

        auto apply_preferences = [this]() {
            std::string home = getenv("HOME") ? getenv("HOME") : "/";
            std::string config_path = home + "/.config/horizon/arkfm.json";
            ConfigManager cfg(config_path);
            if (cfg.load()) {
                auto prefs = cfg.get_section("arkfm");
                if (!prefs.is_null()) {
                    bool show_hidden = prefs.value("show_hidden", false);
                    if (this->m_view_ptr) this->m_view_ptr->set_show_hidden_files(show_hidden);
                    
                    bool show_extensions = prefs.value("show_extensions", true);
                    files::FileIconProvider::set_show_extensions(show_extensions);
                    
                    std::string click_beh = prefs.value("click_behavior", "double");
                    if (this->m_view_ptr) {
                        if (click_beh == "single") this->m_view_ptr->set_click_behavior(files::ClickBehavior::Single);
                        else this->m_view_ptr->set_click_behavior(files::ClickBehavior::Double);
                    }

                    std::string def_view = prefs.value("default_view", "icon");
                    if (this->m_view_ptr) {
                        if (def_view == "table") this->m_view_ptr->set_view_mode(files::ViewMode::List);
                        else if (def_view == "coverflow") this->m_view_ptr->set_view_mode(files::ViewMode::CoverFlow);
                        else this->m_view_ptr->set_view_mode(files::ViewMode::Grid);
                    }
                    
                    if (this->m_view_ptr) this->m_view_ptr->refresh();
                }
            }
        };
        apply_preferences();

        sidebar_ptr->when_item_selected.connect(
            [view_ptr](horizon::SidebarItemSelectedContext &ctx)
            {
                if (!ctx.item->path().empty())
                {
                    view_ptr->navigate_to(ctx.item->path());
                }
            });

        // Initialize Remote Storage from the new library
        m_remote_manager = std::make_unique<storage::RemoteManager>();
        m_sidebar_ptr->set_remote_storage(m_remote_manager.get());

        m_sidebar_ptr->select_item_by_path(start_path);
        ark_toolbar_ptr->update_path(start_path);

        m_view_ptr->when_path_changed.connect(
            [this, ark_toolbar_ptr](files::PathChangedEvent &ctx)
            {
                if (m_sidebar_ptr)
                    m_sidebar_ptr->select_item_by_path(ctx.path);
                if (ark_toolbar_ptr)
                    ark_toolbar_ptr->update_path(ctx.path);
            });

        m_view_ptr->when_operation_progress.connect(
            [this](files::OperationProgressEvent &ctx)
            {
                if (m_progress_bar)
                {
                    m_progress_bar->set_visible(!ctx.finished);
                    m_progress_bar->set_progress(static_cast<float>(ctx.progress));
                }
            });

        m_sidebar_ptr->when_resource_unmounted.connect(
            [this](horizon::files::UnmountEventContext &ctx)
            {
                if (m_view_ptr)
                {
                    std::string current_path = m_view_ptr->current_path();
                    if (!ctx.mount_path.empty() && current_path.find(ctx.mount_path) == 0)
                    {
                        std::string home = getenv("HOME") ? getenv("HOME") : "/";
                        LOG_INFO << "ArkfmWindow: El recurso en " << ctx.mount_path
                                 << " ha sido desmontado. Redirigiendo de " << current_path << " a "
                                 << home;
                        m_view_ptr->navigate_to(home);
                    }
                }
            });

        show_status_bar();
        auto *sb = statusbar();
        auto lbl = std::make_unique<horizon::Label>("");
        m_status_label = lbl.get();

        auto pbc = std::make_unique<horizon::Widget>();
        auto pb = std::make_unique<horizon::ProgressBar>();

        m_progress_bar = pb.get();
        m_progress_bar->set_visible(false);
        m_progress_bar->set_fixed_size(10);

        pbc->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
        pbc->set_fixed_size(200);

        pbc->add_child(horizon::Spacer());
        pbc->add_child(std::move(pb));
        pbc->add_child(horizon::Spacer());

        sb->add_child(horizon::Spacer(10));
        sb->add_child(std::move(lbl));
        sb->add_child(std::move(pbc));
        sb->add_child(horizon::Spacer(10));

        ark_toolbar_ptr->when_navigation_clicked.connect(
            [view_ptr](files::NavigationButtonClickEvent &ctx)
            {
                if (ctx.index == 0)
                {
                    view_ptr->navigate_back();
                }
                else
                {
                    view_ptr->navigate_forward();
                }
            });

        ark_toolbar_ptr->when_view_mode_changed.connect(
            [view_ptr](files::ViewModeChangeEvent &ctx)
            {
                if (ctx.view_mode_index == 0)
                {
                    view_ptr->set_view_mode(files::ViewMode::Grid);
                }
                else if (ctx.view_mode_index == 1)
                {
                    view_ptr->set_view_mode(files::ViewMode::List);
                }
                else if (ctx.view_mode_index == 3)
                {
                    view_ptr->set_view_mode(files::ViewMode::CoverFlow);
                }
            });

        ark_toolbar_ptr->when_search_changed.connect([view_ptr](files::SearchChangedEvent &ctx)
                                                     { view_ptr->set_search_query(ctx.query); });

        ark_toolbar_ptr->when_go_up_clicked.connect(
            [view_ptr](horizon::EventContext &)
            {
                if (view_ptr) {
                    std::filesystem::path p(view_ptr->current_path());
                    view_ptr->navigate_to(p.parent_path().string());
                }
            });

        m_view_ptr->when_item_opened.connect(
            [this](const arkutils::FileInfo &f)
            {
                if (f.type == arkutils::FileType::Regular)
                {
                    if (compression::CompressionManager::is_supported_archive(f.path))
                    {
                        this->handle_extract(f.path);
                    }
                    else
                    {
                        ApplicationLauncher::open_file(f.path);
                    }
                }
            });

        m_view_ptr->when_key_release.connect(
            [this](KeyEventContext &ctx)
            {
                if (ctx.keysym == 0xffff) // Delete key
                {
                    auto sel = m_view_ptr->get_selection();
                    if (!sel.empty() && !m_is_deleting)
                    {
                        std::vector<std::string> paths;
                        for (const auto &item : sel)
                            paths.push_back(item.path);
                        this->handle_delete(paths);
                        ctx.stop_propagation = true;
                    }
                }
                else if (ctx.keysym == 0xff0d) // Enter/Return key
                {
                    auto sel = m_view_ptr->get_selection();
                    if (!sel.empty())
                    {
                        this->m_view_ptr->open_item(sel[0]);
                        ctx.stop_propagation = true;
                    }
                }
            });

        m_view_ptr->set_context_menu_factory(
            [this](const arkutils::FileInfo &f)
            {
                auto menu = std::make_unique<horizon::Menu>();

                auto item_open = menu->add_item(i18n().tr("arkfm.menu.open"));
                item_open->when_click.connect([this, f](auto &)
                                              { this->m_view_ptr->open_item(f); });

                auto item_open_with = menu->add_item(i18n().tr("arkfm.menu.open_with"));
                auto sub_open_with = std::make_unique<horizon::Menu>();

                std::string mime = DesktopManager::get_mime_type(f.path);
                auto apps = DesktopManager::get_apps_for_mime(mime);

                for (const auto &app : apps)
                {
                    auto app_item = sub_open_with->add_item(app.name);
                    if (!app.icon.empty())
                        app_item->set_icon(app.icon);
                    app_item->when_click.connect(
                        [this, f, app](auto &)
                        { ApplicationLauncher::launch_from_desktop_file(app.path, {f.path}); });
                }

                if (!apps.empty())
                    sub_open_with->add_separator();

                auto item_other = sub_open_with->add_item(i18n().tr("arkfm.menu.choose_app"));
                item_other->when_click.connect(
                    [this, f](auto &)
                    {
                        this->application()->post_task(
                            [this, f]()
                            {
                                auto dialog = std::make_unique<AppPickerDialog>();
                                dialog->when_accepted.connect(
                                    [this, f](const DesktopEntry &entry)
                                    {
                                        ApplicationLauncher::launch_from_desktop_file(entry.path,
                                                                                      {f.path});
                                    });
                                dialog->initialize();
                                dialog->run();
                            });
                    });

                item_open_with->set_submenu(std::move(sub_open_with));

                menu->add_separator();

                if (compression::CompressionManager::is_supported_archive(f.path))
                {
                    auto item_extract = menu->add_item(i18n().tr("arkfm.menu.extract_here"));
                    item_extract->when_click.connect([this, f](auto &)
                                                     { this->handle_extract(f.path); });
                    menu->add_separator();
                }
                else
                {
                    auto item_compress = menu->add_item(i18n().tr("arkfm.menu.compress"));
                    auto sub = std::make_unique<horizon::Menu>();

                    auto get_paths_to_compress = [this, f]()
                    {
                        auto sel = this->m_view_ptr->get_selection();
                        bool in_selection = false;
                        std::vector<std::string> paths;
                        for (const auto &item : sel)
                        {
                            paths.push_back(item.path);
                            if (item.path == f.path)
                                in_selection = true;
                        }
                        if (in_selection && paths.size() > 1)
                        {
                            return paths;
                        }
                        return std::vector<std::string>{f.path};
                    };

                    auto zip = sub->add_item(".zip");
                    zip->when_click.connect(
                        [this, get_paths_to_compress](auto &)
                        { this->handle_compress(get_paths_to_compress(), ".zip"); });

                    auto targz = sub->add_item(".tar.gz");
                    targz->when_click.connect(
                        [this, get_paths_to_compress](auto &)
                        { this->handle_compress(get_paths_to_compress(), ".tar.gz"); });

                    auto sevenz = sub->add_item(".7z");
                    sevenz->when_click.connect(
                        [this, get_paths_to_compress](auto &)
                        { this->handle_compress(get_paths_to_compress(), ".7z"); });

                    item_compress->set_submenu(std::move(sub));
                    menu->add_separator();
                }

                auto item_rename = menu->add_item(i18n().tr("arkfm.menu.rename"));
                item_rename->when_click.connect(
                    [this, f](auto &)
                    {
                        this->application()->post_task([this, f]()
                                                       { this->handle_rename(f.path); });
                    });
                    
                if (f.type == horizon::arkutils::FileType::Directory)
                {
                    menu->add_separator();
                    auto item_fav = menu->add_item(i18n().tr("arkfm.menu.add_bookmark"));
                    item_fav->when_click.connect([this, f](auto &)
                                                 {
                                                     this->application()->post_task([this, f]()
                                                                                    { this->handle_add_bookmark(f.path); });
                                                 });
                }

                bool is_in_trash =
                    m_view_ptr->current_path().find("/.local/share/Trash") != std::string::npos;
                if (!is_in_trash)
                {
                    auto item_trash = menu->add_item(i18n().tr("arkfm.menu.move_to_trash"));
                    item_trash->when_click.connect(
                        [this, f](auto &)
                        {
                            this->application()->post_task(
                                [this, f]()
                                {
                                    auto sel = this->m_view_ptr->get_selection();
                                    bool in_selection = false;
                                    std::vector<std::string> paths;
                                    for (const auto &item : sel)
                                    {
                                        paths.push_back(item.path);
                                        if (item.path == f.path)
                                            in_selection = true;
                                    }
                                    if (in_selection && paths.size() > 1)
                                    {
                                        this->handle_trash(paths);
                                    }
                                    else
                                    {
                                        this->handle_trash({f.path});
                                    }
                                });
                        });
                }
                else
                {
                    auto item_restore = menu->add_item(i18n().tr("arkfm.menu.restore"));
                    item_restore->when_click.connect(
                        [this, f](auto &)
                        {
                            this->application()->post_task(
                                [this, f]()
                                {
                                    auto sel = this->m_view_ptr->get_selection();
                                    bool in_selection = false;
                                    std::vector<std::string> paths;
                                    for (const auto &item : sel)
                                    {
                                        paths.push_back(item.path);
                                        if (item.path == f.path)
                                            in_selection = true;
                                    }
                                    if (in_selection && paths.size() > 1)
                                    {
                                        this->handle_restore(paths);
                                    }
                                    else
                                    {
                                        this->handle_restore({f.path});
                                    }
                                });
                        });
                }

                auto item_delete = menu->add_item(i18n().tr("arkfm.menu.delete"));
                item_delete->when_click.connect(
                    [this, f](auto &)
                    {
                        this->application()->post_task(
                            [this, f]()
                            {
                                auto sel = this->m_view_ptr->get_selection();
                                bool in_selection = false;
                                std::vector<std::string> paths;
                                for (const auto &item : sel)
                                {
                                    paths.push_back(item.path);
                                    if (item.path == f.path)
                                        in_selection = true;
                                }
                                if (in_selection && paths.size() > 1)
                                {
                                    this->handle_delete(paths);
                                }
                                else
                                {
                                    this->handle_delete({f.path});
                                }
                            });
                    });

                menu->add_separator();

                auto item_props = menu->add_item(i18n().tr("arkfm.menu.properties"));
                item_props->when_click.connect(
                    [this, f](auto &)
                    {
                        auto dialog = std::make_unique<PropertiesDialog>(f);
                        dialog->run();
                    });

                menu->add_separator();
                auto item_terminal = menu->add_item(i18n().tr("arkfm.menu.open_terminal"));
                item_terminal->when_click.connect(
                    [this, f](auto &)
                    {
                        std::string term_path = (f.type == horizon::arkutils::FileType::Directory)
                                                    ? f.path
                                                    : m_view_ptr->current_path();
                        ApplicationLauncher::launch_binary("terminal", {}, term_path);
                    });

                menu->add_separator();
                auto item_connect = menu->add_item(i18n().tr("arkfm.menu.connect_to_server"));
                item_connect->when_click.connect(
                    [this](auto &) { application()->signal_manager.emit("go-connect"); });

                menu->add_separator();
                std::string hidden_text = m_view_ptr->show_hidden_files()
                                              ? i18n().tr("arkfm.menu.hide_hidden")
                                              : i18n().tr("arkfm.menu.show_hidden");
                auto item_show_hidden = menu->add_item(hidden_text, "Ctrl+H");
                item_show_hidden->when_click.connect([this](auto &)
                                                     { this->handle_toggle_hidden(); });

                return menu;
            });

        vpanel->add_child(std::move(sidebar));
        vpanel->add_child(std::move(view));
        set_content(std::move(vpanel));

        this->when_application_load.connect(
            [this, view_ptr, apply_preferences](EventContext &)
            {
                if (application())
                {
                    application()->signal_manager.connect("preferences-changed", [this, apply_preferences](SignalContext &) {
                        if (this->application()) {
                            this->application()->post_task([apply_preferences]() {
                                apply_preferences();
                            });
                        }
                    });

                    application()->signal_manager.connect("new-folder", [this](SignalContext &)
                                                          { this->handle_new_folder(); });

                    application()->signal_manager.connect("toggle-hidden", [this](SignalContext &)
                                                          { this->handle_toggle_hidden(); });

                    application()->signal_manager.connect("delete",
                                                          [this](SignalContext &)
                                                          {
                                                              auto sel =
                                                                  m_view_ptr->get_selection();
                                                              if (!sel.empty())
                                                              {
                                                                  std::vector<std::string> paths;
                                                                  for (const auto &item : sel)
                                                                      paths.push_back(item.path);
                                                                  handle_delete(paths);
                                                              }
                                                          });
                    application()->signal_manager.connect("properties", [this](SignalContext &)
                                                          { handle_properties(); });

                    // Go Menu Handlers
                    application()->signal_manager.connect("go-back",
                                                          [this](SignalContext &)
                                                          {
                                                              if (m_view_ptr)
                                                                  m_view_ptr->navigate_back();
                                                          });
                    application()->signal_manager.connect("go-forward",
                                                          [this](SignalContext &)
                                                          {
                                                              if (m_view_ptr)
                                                                  m_view_ptr->navigate_forward();
                                                          });
                    application()->signal_manager.connect(
                        "go-parent",
                        [this](SignalContext &)
                        {
                            if (m_view_ptr)
                            {
                                std::filesystem::path p(m_view_ptr->current_path());
                                m_view_ptr->navigate_to(p.parent_path().string());
                            }
                        });
                    application()->signal_manager.connect(
                        "go-home",
                        [this](SignalContext &)
                        {
                            if (m_view_ptr)
                                m_view_ptr->navigate_to(getenv("HOME") ? getenv("HOME") : "/");
                        });
                    application()->signal_manager.connect(
                        "go-documents",
                        [this](SignalContext &)
                        {
                            if (m_view_ptr)
                                m_view_ptr->navigate_to(
                                    std::string(getenv("HOME") ? getenv("HOME") : "/") +
                                    "/Documents");
                        });
                    application()->signal_manager.connect(
                        "go-desktop",
                        [this](SignalContext &)
                        {
                            if (m_view_ptr)
                                m_view_ptr->navigate_to(
                                    std::string(getenv("HOME") ? getenv("HOME") : "/") +
                                    "/Desktop");
                        });
                    application()->signal_manager.connect(
                        "go-downloads",
                        [this](SignalContext &)
                        {
                            if (m_view_ptr)
                                m_view_ptr->navigate_to(
                                    std::string(getenv("HOME") ? getenv("HOME") : "/") +
                                    "/Downloads");
                        });
                    application()->signal_manager.connect(
                        "go-videos",
                        [this](SignalContext &)
                        {
                            if (m_view_ptr)
                                m_view_ptr->navigate_to(
                                    std::string(getenv("HOME") ? getenv("HOME") : "/") + "/Videos");
                        });
                    application()->signal_manager.connect(
                        "go-images",
                        [this](SignalContext &)
                        {
                            if (m_view_ptr)
                                m_view_ptr->navigate_to(
                                    std::string(getenv("HOME") ? getenv("HOME") : "/") + "/Images");
                        });
                    application()->signal_manager.connect("go-applications",
                                                          [this](SignalContext &)
                                                          {
                                                              if (m_view_ptr)
                                                                  m_view_ptr->navigate_to(
                                                                      "/usr/share/applications");
                                                          });
                    application()->signal_manager.connect(
                        "go-to-folder",
                        [this](SignalContext &)
                        {
                            application()->post_task(
                                [this]()
                                {
                                    application()->set_override_cursor(CursorType::Wait);
                                    auto dialog = std::make_unique<GoToFolderDialog>();
                                    dialog->when_accepted.connect(
                                        [this](GoToFolderEvent &ev)
                                        {
                                            if (m_view_ptr)
                                                m_view_ptr->navigate_to(ev.path);
                                        });
                                    dialog->run();
                                    application()->clear_override_cursor();
                                });
                        });

                    application()->signal_manager.connect(
                        "go-connect",
                        [this](SignalContext &)
                        {
                            application()->post_task(
                                [this]()
                                {
                                    application()->set_override_cursor(CursorType::Wait);
                                    auto dialog = std::make_unique<ConnectToServerDialog>();
                                    dialog->when_accepted.connect(
                                        [this](ConnectToServerEvent &ev)
                                        { this->handle_mount_remote(ev.uri); });
                                    dialog->run();
                                    application()->clear_override_cursor();
                                });
                        });

                    m_view_ptr->when_right_click.connect(
                        [this](MouseButtonEventContext &ctx)
                        {
                            if (!ctx.stop_propagation)
                            {
                                m_active_context_menu = std::make_unique<horizon::Menu>();

                                bool is_in_trash = m_view_ptr->current_path().find(
                                                       "/.local/share/Trash") != std::string::npos;
                                if (is_in_trash)
                                {
                                    auto item_empty_trash = m_active_context_menu->add_item(
                                        "Vaciar papelera", "edit-delete");
                                    item_empty_trash->when_click.connect(
                                        [this](auto &)
                                        {
                                            this->application()->post_task(
                                                [this]() { this->handle_empty_trash(); });
                                        });
                                    m_active_context_menu->add_separator();
                                }

                                auto item_new = m_active_context_menu->add_item(i18n().tr("arkfm.menu.new_folder"));
                                item_new->when_click.connect([this](auto &)
                                                             { this->handle_new_folder(); });

                                m_active_context_menu->add_separator();

                                auto item_props = m_active_context_menu->add_item(i18n().tr("arkfm.menu.properties"));
                                item_props->when_click.connect([this](auto &)
                                                               { this->handle_properties(); });

                                m_active_context_menu->add_separator();
                                auto item_terminal = m_active_context_menu->add_item(
                                    i18n().tr("arkfm.menu.open_terminal"));
                                item_terminal->when_click.connect(
                                    [this](auto &)
                                    {
                                        ApplicationLauncher::launch_binary(
                                            "terminal", {}, m_view_ptr->current_path());
                                    });

                                m_active_context_menu->add_separator();

                                auto item_connect =
                                    m_active_context_menu->add_item(i18n().tr("arkfm.menu.connect_to_server"));
                                item_connect->when_click.connect(
                                    [this](auto &)
                                    { application()->signal_manager.emit("go-connect"); });

                                m_active_context_menu->add_separator();

                                auto item_show_hidden = m_active_context_menu->add_item(
                                    m_view_ptr->show_hidden_files()
                                        ? i18n().tr("arkfm.menu.hide_hidden")
                                        : i18n().tr("arkfm.menu.show_hidden"),
                                    "Ctrl+H");
                                item_show_hidden->when_click.connect(
                                    [this](auto &) { this->handle_toggle_hidden(); });

                                application()->show_context_menu(m_active_context_menu.get(), -1,
                                                                 -1, ctx.serial, this->m_view_ptr);
                                ctx.stop_propagation = true;
                            }
                        });

                    application()->when_popup_dismissed.connect([this](PopupDismissedContext &)
                                                                { m_active_context_menu.reset(); });
                }
            });
    }

    ArkfmWindow::~ArkfmWindow() = default;

    void ArkfmWindow::handle_toggle_hidden()
    {
        if (m_view_ptr)
        {
            m_view_ptr->set_show_hidden_files(!m_view_ptr->show_hidden_files());

            // Update the global View menu item text dynamically
            if (application())
            {
                if (auto *view_menu = application()->get_menu("view"))
                {
                    for (auto &child : view_menu->children())
                    {
                        if (auto *item = dynamic_cast<horizon::MenuItem *>(child.get()))
                        {
                            if (item->id() == "toggle-hidden")
                            {
                                item->set_text(m_view_ptr->show_hidden_files()
                                                   ? i18n().tr("arkfm.menu.hide_hidden")
                                                   : i18n().tr("arkfm.menu.show_hidden"));
                                break;
                            }
                        }
                    }
                    application()->update_global_menu();
                }
            }
        }
    }

    void ArkfmWindow::handle_new_folder()
    {
        if (!m_view_ptr)
            return;

        application()->post_task(
            [this]()
            {
                application()->set_override_cursor(CursorType::Wait);
                auto dialog = std::make_unique<NewFolderDialog>();
                dialog->when_accepted.connect(
                    [this](NewFolderEvent &ctx)
                    {
                        std::string full_path = m_view_ptr->current_path() + "/" + ctx.folder_name;
                        show_status_message(i18n().tr("arkfm.messages.creating_folder"));
                        auto future = arkutils::FileOperations::create_directory(full_path);
                        std::thread(
                            [this, f = std::move(future)]() mutable
                            {
                                auto result = f.get();
                                if (application())
                                {
                                    application()->post_task(
                                        [this, result]()
                                        {
                                            if (result == arkutils::FileOperations::Result::Success)
                                            {
                                                show_status_message(i18n().tr("arkfm.messages.folder_created"));
                                                if (m_view_ptr)
                                                    m_view_ptr->refresh();
                                            }
                                            else
                                            {
                                                application()->alert("No se pudo crear la carpeta.",
                                                                     "Error", MessageType::Error);
                                            }
                                        });
                                }
                            })
                            .detach();
                    });
                dialog->run();
                application()->clear_override_cursor();
            });
    }
    
    void ArkfmWindow::handle_add_bookmark(const std::string &path)
    {
        auto home = getenv("HOME") ? getenv("HOME") : "/home/user";
        std::string bookmarks_path = std::string(home) + "/.config/gtk-3.0/bookmarks";
        
        std::filesystem::path config_dir = std::string(home) + "/.config/gtk-3.0";
        if (!std::filesystem::exists(config_dir))
        {
            std::filesystem::create_directories(config_dir);
        }

        std::string encoded_uri = "file://";
        for (char c : path) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
                encoded_uri += c;
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
                encoded_uri += buf;
            }
        }
        
        std::ofstream file(bookmarks_path, std::ios_base::app);
        if (file.is_open())
        {
            file << encoded_uri << "\n";
            file.close();
            
            show_status_message(i18n().tr("arkfm.messages.added_to_bookmarks"));
            
            if (m_sidebar_ptr)
                m_sidebar_ptr->refresh_devices();
        }
    }

    void ArkfmWindow::handle_rename(const std::string &path)
    {
        application()->set_override_cursor(CursorType::Wait);
        std::filesystem::path p(path);
        auto dialog = std::make_unique<RenameDialog>(p.filename().string());
        dialog->when_accepted.connect(
            [this, path, p](RenameEvent &ctx)
            {
                std::filesystem::path new_path = p.parent_path() / ctx.new_name;

                if (std::filesystem::exists(new_path))
                {
                    application()->alert("Ya existe un archivo o carpeta con el nombre '" +
                                             ctx.new_name + "' en esta ubicación.",
                                         i18n().tr("arkfm.messages.rename_error"), MessageType::Error);
                    return;
                }

                auto future = arkutils::FileOperations::rename(path, new_path.string());
                std::thread(
                    [this, f = std::move(future)]() mutable
                    {
                        auto result = f.get();
                        if (application())
                        {
                            application()->post_task(
                                [this, result]()
                                {
                                    if (result == arkutils::FileOperations::Result::Success)
                                    {
                                        show_status_message(i18n().tr("arkfm.messages.rename_success"));
                                        if (m_view_ptr)
                                            m_view_ptr->refresh();
                                    }
                                    else
                                    {
                                        application()->alert(
                                            "No se pudo renombrar el archivo o carpeta.", "Error",
                                            MessageType::Error);
                                    }
                                });
                        }
                    })
                    .detach();
            });
        dialog->run();
        application()->clear_override_cursor();
    }

    void ArkfmWindow::handle_delete(const std::vector<std::string> &paths)
    {
        if (m_is_deleting || paths.empty())
            return;
        m_is_deleting = true;

        application()->set_override_cursor(CursorType::Wait);

        std::string prompt_msg;
        if (paths.size() == 1)
        {
            std::string filename = std::filesystem::path(paths[0]).filename().string();
            prompt_msg = i18n().tr("arkfm.messages.delete_prompt_single");
            size_t pos = prompt_msg.find("%1");
            if (pos != std::string::npos) prompt_msg.replace(pos, 2, filename);
        }
        else
        {
            prompt_msg = i18n().tr("arkfm.messages.delete_prompt_multiple");
            size_t pos = prompt_msg.find("%1");
            if (pos != std::string::npos) prompt_msg.replace(pos, 2, std::to_string(paths.size()));
        }

        if (application()->confirm(prompt_msg, "Confirmar eliminación"))
        {
            show_status_message(i18n().tr("arkfm.messages.deleting"));
            std::thread(
                [this, paths]() mutable
                {
                    bool all_success = true;
                    for (const auto &path : paths)
                    {
                        auto future = arkutils::FileOperations::remove(path);
                        auto result = future.get();
                        if (result != arkutils::FileOperations::Result::Success)
                        {
                            all_success = false;
                        }
                    }

                    if (application())
                    {
                        application()->post_task(
                            [this, all_success]()
                            {
                                if (all_success)
                                {
                                    show_status_message(i18n().tr("arkfm.messages.delete_success"));
                                }
                                else
                                {
                                    application()->alert(
                                        i18n().tr("arkfm.messages.delete_error_multi"),
                                        "Error", MessageType::Error);
                                }
                                if (m_view_ptr)
                                    m_view_ptr->refresh();
                            });
                    }
                })
                .detach();
        }
        application()->clear_override_cursor();
        m_is_deleting = false;
    }

    void ArkfmWindow::handle_trash(const std::vector<std::string> &paths)
    {
        if (m_is_deleting || paths.empty())
            return;
        m_is_deleting = true;

        application()->set_override_cursor(CursorType::Wait);

        show_status_message(i18n().tr("arkfm.messages.moving_to_trash"));
        std::thread(
            [this, paths]() mutable
            {
                bool all_success = true;
                for (const auto &path : paths)
                {
                    // Ejecutar gio trash para mover a la papelera
                    std::string cmd = "gio trash \"" + path + "\"";
                    int result = system(cmd.c_str());
                    if (result != 0)
                    {
                        all_success = false;
                    }
                }

                if (application())
                {
                    application()->post_task(
                        [this, all_success]()
                        {
                            if (all_success)
                            {
                                show_status_message(i18n().tr("arkfm.messages.moved_to_trash_success"));
                            }
                            else
                            {
                                application()->alert(
                                    i18n().tr("arkfm.messages.trash_error_multi"),
                                    "Error", MessageType::Error);
                            }
                            if (m_view_ptr)
                                m_view_ptr->refresh();
                        });
                }
            })
            .detach();

        application()->clear_override_cursor();
        m_is_deleting = false;
    }

    void ArkfmWindow::handle_restore(const std::vector<std::string> &paths)
    {
        if (m_is_deleting || paths.empty())
            return;
        m_is_deleting = true;

        application()->set_override_cursor(CursorType::Wait);

        show_status_message(i18n().tr("arkfm.messages.restoring_from_trash"));
        std::thread(
            [this, paths]() mutable
            {
                bool all_success = true;
                for (const auto &path : paths)
                {
                    std::filesystem::path p(path);
                    std::string filename = p.filename().string();
                    std::string cmd = "gio trash --restore \"trash:///" + filename + "\"";
                    int result = system(cmd.c_str());
                    if (result != 0)
                    {
                        all_success = false;
                    }
                }

                if (application())
                {
                    application()->post_task(
                        [this, all_success]()
                        {
                            if (all_success)
                            {
                                show_status_message(i18n().tr("arkfm.messages.restore_success"));
                            }
                            else
                            {
                                application()->alert(
                                    i18n().tr("arkfm.messages.restore_error_multi"),
                                    "Error", MessageType::Error);
                            }
                            if (m_view_ptr)
                                m_view_ptr->refresh();
                        });
                }
            })
            .detach();

        application()->clear_override_cursor();
        m_is_deleting = false;
    }

    void ArkfmWindow::handle_empty_trash()
    {
        if (m_is_deleting)
            return;

        if (application()->confirm(
                "¿Está seguro que desea vaciar la papelera? Esta acción no se puede deshacer.",
                "Confirmar vaciar papelera"))
        {
            m_is_deleting = true;
            application()->set_override_cursor(CursorType::Wait);

            show_status_message(i18n().tr("arkfm.messages.emptying_trash"));
            std::thread(
                [this]() mutable
                {
                    std::string cmd = "gio trash --empty";
                    int result = system(cmd.c_str());

                    if (application())
                    {
                        application()->post_task(
                            [this, result]()
                            {
                                if (result == 0)
                                {
                                    show_status_message(i18n().tr("arkfm.messages.empty_trash_success"));
                                }
                                else
                                {
                                    application()->alert(i18n().tr("arkfm.messages.empty_trash_error"),
                                                         "Error", MessageType::Error);
                                }
                                if (m_view_ptr)
                                    m_view_ptr->refresh();
                            });
                    }
                })
                .detach();

            application()->clear_override_cursor();
            m_is_deleting = false;
        }
    }

    void ArkfmWindow::handle_open()
    {
        if (m_view_ptr)
            m_view_ptr->open_selection();
    }

    void ArkfmWindow::handle_properties()
    {
        if (!m_view_ptr)
            return;

        auto sel = m_view_ptr->get_selection();
        arkutils::FileInfo f;
        if (sel.empty())
        {
            f.name = std::filesystem::path(m_view_ptr->current_path()).filename().string();
            if (f.name.empty())
                f.name = "/";
            f.path = m_view_ptr->current_path();
            f.type = arkutils::FileType::Directory;
        }
        else
        {
            f = sel[0];
        }

        application()->post_task(
            [this, f]()
            {
                application()->set_override_cursor(CursorType::Wait);
                auto dialog = std::make_unique<PropertiesDialog>(f);
                dialog->run();
                application()->clear_override_cursor();
            });
    }

    void ArkfmWindow::show_status_message(const std::string &msg, int timeout_ms)
    {
        if (!m_status_label)
            return;
        m_status_label->set_text(msg);

        if (application() && timeout_ms > 0)
        {
            application()->add_timer(timeout_ms,
                                     [this, msg]()
                                     {
                                         if (m_status_label && m_status_label->text() == msg)
                                         {
                                             m_status_label->set_text("");
                                         }
                                     });
        }
    }

    void ArkfmWindow::handle_mount_remote(const std::string &uri, storage::RemoteCredentials creds,
                                          storage::MountPasswordDialog *dlg)
    {
        application()->set_override_cursor(CursorType::Wait);
        LOG_INFO << "ArkFM: Intentando montar " << uri;
        if (!m_remote_manager)
        {
            m_remote_manager = std::make_unique<storage::RemoteManager>();
            m_sidebar_ptr->set_remote_storage(m_remote_manager.get());
        }

        // Check if already mounted
        auto active_mounts = m_remote_manager->get_active_mounts();

        // Check if we have saved credentials for this URI
        if (creds.username.empty() && !creds.is_guest)
        {
            storage::RemoteCredentials saved;
            if (m_remote_manager->get_credentials(uri, saved))
            {
                LOG_INFO << "ArkfmWindow: Utilizando credenciales guardadas para " << uri;
                creds = saved;
            }
        }

        for (const auto &mount : active_mounts)
        {
            std::string n_uri = uri;
            if (!n_uri.empty() && n_uri.back() == '/')
                n_uri.pop_back();
            std::string m_uri = mount.uri;
            if (!m_uri.empty() && m_uri.back() == '/')
                m_uri.pop_back();

            if (n_uri == m_uri)
            {
                LOG_INFO << "ArkfmWindow: URI ya montada en " << mount.mount_path;
                application()->clear_override_cursor();
                if (!mount.mount_path.empty())
                {
                    if (m_view_ptr)
                        m_view_ptr->navigate_to(mount.mount_path);
                    if (m_sidebar_ptr)
                        m_sidebar_ptr->select_item_by_path(mount.mount_path);
                }
                else
                {
                    LOG_WARNING << "ArkfmWindow: URI montada pero sin ruta FUSE.";
                }

                if (application())
                {
                    application()->alert(i18n().tr("arkfm.messages.already_mounted"));
                }
                return;
            }
        }

        // Capturamos un weak_ptr para saber si el diálogo sigue vivo de forma segura
        std::weak_ptr<storage::MountPasswordDialog> weak_dlg = m_mount_dialog;

        m_remote_manager->when_mount(
            uri, creds,
            [this, uri, weak_dlg, creds](storage::RemoteMountResult res)
            {
                auto shared_dlg = weak_dlg.lock();

                // ¡ESTO ES LO IMPORTANTE!
                // Si hay diálogo, enviamos la tarea a SU cola. Si no, a la de la App.
                auto task_launcher = [this, shared_dlg](std::function<void()> task)
                {
                    if (shared_dlg)
                        shared_dlg->post_task(task);
                    else
                        application()->post_task(task);
                };

                task_launcher(
                    [this, uri, res, shared_dlg, creds]()
                    {
                        application()->clear_override_cursor();
                        if (res.success)
                        {
                            LOG_INFO << "ArkFM: Montado exitoso de " << uri
                                     << ". Cerrando diálogo...";
                            if (shared_dlg)
                            {
                                shared_dlg->quit();
                                shared_dlg->wakeup();
                            }
                            // NO reseteamos aquí para evitar destruir el objeto mientras su loop
                            // corre

                            // IMPORTANTE: La navegación la hace la ventana principal, no el
                            // diálogo.
                            application()->post_task(
                                [this, res, uri, creds]()
                                {
                                    if (!res.mount_path.empty())
                                    {
                                        // If successfully mounted and 'remember' was checked, save
                                        // credentials
                                        if (creds.remember)
                                        {
                                            m_remote_manager->save_credentials(uri, creds);
                                        }

                                        if (m_view_ptr)
                                            m_view_ptr->navigate_to(res.mount_path);
                                        if (m_sidebar_ptr)
                                        {
                                            application()->add_timer(
                                                1000,
                                                [this, path = res.mount_path]()
                                                {
                                                    if (m_sidebar_ptr)
                                                    {
                                                        m_sidebar_ptr->refresh_devices();
                                                        m_sidebar_ptr->select_item_by_path(path);
                                                    }
                                                },
                                                false);
                                        }
                                        show_status_message("Conectado a " + res.mount_path);
                                    }
                                    else
                                    {
                                        LOG_WARNING << "ArkfmWindow: Montaje exitoso pero sin ruta "
                                                       "FUSE para "
                                                    << uri;
                                        application()->alert(
                                            "Conectado con éxito, pero no se encontró un punto de "
                                            "montaje local. Verifique que la ubicación sea un "
                                            "recurso compartido válido.",
                                            "Aviso");
                                    }
                                });
                        }
                        else
                        {
                            LOG_ERROR << "ArkFM: Fallo al montar " << uri << ": " << res.message;

                            std::string msg = res.message;
                            std::string raw_msg = msg;
                            std::transform(msg.begin(), msg.end(), msg.begin(), ::tolower);

                            bool needs_auth = (msg.find("not authorized") != std::string::npos ||
                                               msg.find("password") != std::string::npos ||
                                               msg.find("contrase") != std::string::npos ||
                                               msg.find("access denied") != std::string::npos ||
                                               msg.find("permission denied") != std::string::npos ||
                                               msg.find("authentication") != std::string::npos ||
                                               msg.find("autentica") != std::string::npos ||
                                               msg.find("soportada") != std::string::npos ||
                                               msg.find("supported") != std::string::npos ||
                                               msg.find("handshake") != std::string::npos ||
                                               msg.find("refused") != std::string::npos);

                            if (needs_auth)
                            {
                                if (shared_dlg)
                                {
                                    LOG_INFO << "ArkFM: Informando error al diálogo existente.";

                                    std::string display_msg = raw_msg;
                                    // Si el error es "cancelado" o "no soportada", usamos nuestro
                                    // mensaje genérico amigable
                                    if (msg.find("cancel") != std::string::npos ||
                                        msg.find("soportada") != std::string::npos ||
                                        msg.find("supported") != std::string::npos)
                                    {
                                        display_msg = horizon::i18n().tr(
                                            "core.storage.mount_dialog.auth_error");
                                    }

                                    shared_dlg->show_error(display_msg);
                                }
                                else
                                {
                                    LOG_INFO << "ArkFM: Creando nuevo diálogo de contraseña.";
                                    m_mount_dialog =
                                        std::make_shared<storage::MountPasswordDialog>(uri);
                                    m_mount_dialog->set_initial_credentials(creds);

                                    // Capturamos un weak_ptr para evitar una referencia circular
                                    std::weak_ptr<storage::MountPasswordDialog> weak_dlg_signal =
                                        m_mount_dialog;

                                    m_mount_dialog->when_accepted.connect(
                                        [this, uri,
                                         weak_dlg_signal](storage::MountPasswordEvent &ev)
                                        {
                                            if (auto shared_dlg_signal = weak_dlg_signal.lock())
                                            {
                                                this->handle_mount_remote(uri, ev.credentials,
                                                                          shared_dlg_signal.get());
                                            }
                                        });

                                    m_mount_dialog->run();
                                    m_mount_dialog.reset();
                                    LOG_INFO << "ArkFM: El bucle del diálogo ha finalizado.";
                                }
                            }
                            else
                            {
                                std::string display_msg = raw_msg;
                                if (msg.find("cancel") != std::string::npos ||
                                    msg.find("soportada") != std::string::npos ||
                                    msg.find("supported") != std::string::npos)
                                {
                                    display_msg =
                                        horizon::i18n().tr("core.storage.mount_dialog.auth_error");
                                }

                                if (shared_dlg)
                                    shared_dlg->show_error(display_msg);
                                show_status_message("Error: " + display_msg, 5000);

                                // Show alert for real errors (excluding cancellations by user)
                                if (msg.find("cancel") == std::string::npos && application())
                                {
                                    application()->alert(display_msg,
                                                         i18n().tr("arkfm.dialog.error") !=
                                                                 "arkfm.dialog.error"
                                                             ? i18n().tr("arkfm.dialog.error")
                                                             : "Error");
                                }
                            }
                        }
                    });

                if (shared_dlg)
                    shared_dlg->wakeup();
            });
    }

    void ArkfmWindow::handle_extract(const std::string &path)
    {
        std::string dest = m_view_ptr->current_path();
        show_status_message(i18n().tr("arkfm.messages.extracting"));

        auto task = compression::CompressionManager::extract_smart(path, dest);
        m_active_tasks.push_back(task);

        task->when_progress.connect(
            [this](compression::CompressionProgressEvent &ev)
            {
                if (m_progress_bar)
                {
                    m_progress_bar->set_visible(true);
                    m_progress_bar->set_progress(static_cast<float>(ev.progress));
                }
                if (!ev.status_message.empty())
                    show_status_message(ev.status_message);
            });

        task->when_finished.connect(
            [this, path, task](compression::CompressionFinishedEvent &ev)
            {
                application()->post_task(
                    [this, ev, path, task]()
                    {
                        if (m_progress_bar)
                            m_progress_bar->set_visible(false);

                        if (ev.success)
                        {
                            show_status_message(i18n().tr("arkfm.messages.extract_success"));
                            NotificationSender::send("Extracción completada",
                                                     "El archivo se ha extraído correctamente.",
                                                     "package-x-generic");
                            if (m_view_ptr)
                                m_view_ptr->refresh();
                        }
                        else
                        {
                            show_status_message(i18n().tr("arkfm.messages.extract_error"));
                            application()->alert(i18n().tr("arkfm.messages.extract_error") + ": " + ev.error_message,
                                                 "Error", MessageType::Error);
                        }

                        // Remove from active tasks
                        auto it = std::find(m_active_tasks.begin(), m_active_tasks.end(), task);
                        if (it != m_active_tasks.end())
                            m_active_tasks.erase(it);
                    });
            });

        task->start();
    }

    void ArkfmWindow::handle_compress(const std::vector<std::string> &paths,
                                      const std::string &format_ext)
    {
        if (paths.empty())
            return;

        std::filesystem::path first(paths[0]);
        std::string base_name;
        std::string out_path;

        if (paths.size() > 1)
        {
            base_name = first.parent_path().filename().string();
            if (base_name.empty() || base_name == "/")
                base_name = "Archive";
        }
        else
        {
            base_name = first.stem().string();
        }

        out_path = (first.parent_path() / (base_name + format_ext)).string();
        int counter = 1;
        while (std::filesystem::exists(out_path))
        {
            out_path =
                (first.parent_path() / (base_name + "_" + std::to_string(counter) + format_ext))
                    .string();
            counter++;
        }

        show_status_message(i18n().tr("arkfm.messages.compressing"));

        compression::ArchiveFormat fmt =
            compression::CompressionManager::format_from_extension(format_ext);
        auto task = compression::CompressionManager::compress(paths, out_path, fmt);
        m_active_tasks.push_back(task);

        task->when_progress.connect(
            [this](compression::CompressionProgressEvent &ev)
            {
                if (m_progress_bar)
                {
                    m_progress_bar->set_visible(true);
                    m_progress_bar->set_progress(static_cast<float>(ev.progress));
                }
            });

        task->when_finished.connect(
            [this, out_path, task](compression::CompressionFinishedEvent &ev)
            {
                application()->post_task(
                    [this, ev, out_path, task]()
                    {
                        if (m_progress_bar)
                            m_progress_bar->set_visible(false);

                        if (ev.success)
                        {
                            show_status_message(i18n().tr("arkfm.messages.compress_success"));
                            NotificationSender::send("Compresión completada",
                                                     "El archivo se ha creado correctamente.",
                                                     "package-x-generic");
                            if (m_view_ptr)
                                m_view_ptr->refresh();
                        }
                        else
                        {
                            show_status_message(i18n().tr("arkfm.messages.compress_error"));
                            application()->alert(i18n().tr("arkfm.messages.compress_error") + ": " + ev.error_message, "Error",
                                                 MessageType::Error);
                        }

                        // Remove from active tasks
                        auto it = std::find(m_active_tasks.begin(), m_active_tasks.end(), task);
                        if (it != m_active_tasks.end())
                            m_active_tasks.erase(it);
                    });
            });

        task->start();
    }

} // namespace horizon::arkfm