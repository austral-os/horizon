#include <horizon/files/FileContextMenuBuilder.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/I18n.hpp>
#include <horizon/files/dialogs/PropertiesDialog.hpp>
#include <horizon/files/dialogs/RenameDialog.hpp>
#include <horizon/files/dialogs/NewFolderDialog.hpp>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/DesktopManager.hpp>
#include <horizon/dialogs/AppPickerDialog.hpp>
#include <filesystem>
#include <system_error>

namespace horizon::files
{
    std::unique_ptr<Menu> FileContextMenuBuilder::build_item_menu(
        const arkutils::FileInfo &f, 
        const Callbacks &callbacks)
    {
        auto menu = std::make_unique<Menu>();

        auto item_open = menu->add_item(i18n().tr("core.file_menu.open"));
        item_open->when_click.connect([f](auto &) {
            ApplicationLauncher::open_file(f.path);
        });

        if (f.type != arkutils::FileType::Directory) {
            auto item_open_with = menu->add_item(i18n().tr("core.file_menu.open_with"));
            auto sub_open_with = std::make_unique<Menu>();
            
            std::string mime = DesktopManager::get_mime_type(f.path);
            auto apps = DesktopManager::get_apps_for_mime(mime);
            
            for (const auto &app : apps) {
                auto sub_item = sub_open_with->add_item(app.name);
                if (!app.icon.empty()) sub_item->set_icon(app.icon);
                sub_item->when_click.connect([app, f](auto &) {
                    ApplicationLauncher::launch_from_desktop_file(app.path, {f.path});
                });
            }
            
            if (!apps.empty()) sub_open_with->add_separator();
            
            auto item_other = sub_open_with->add_item(i18n().tr("core.file_menu.choose_app"));
            item_other->when_click.connect([f](horizon::MouseButtonEventContext &ev) {
                if (auto app = static_cast<horizon::Widget*>(ev.sender)->application()) {
                    app->add_timer(50, [f]() {
                        auto dialog = std::make_unique<AppPickerDialog>();
                        dialog->when_accepted.connect([f](const DesktopEntry& app) {
                            ApplicationLauncher::launch_from_desktop_file(app.path, {f.path});
                        });
                        dialog->run();
                    }, false);
                }
            });
            item_open_with->set_submenu(std::move(sub_open_with));
            menu->add_separator();
        }

        auto item_rename = menu->add_item(i18n().tr("core.file_menu.rename"));
        item_rename->when_click.connect([f, callbacks](horizon::MouseButtonEventContext &ev) {
            if (auto app = static_cast<horizon::Widget*>(ev.sender)->application()) {
                app->add_timer(50, [f, callbacks]() {
                    auto dialog = std::make_unique<RenameDialog>(f.name);
                    dialog->when_accepted.connect([f, callbacks](auto& ev2) {
                        std::filesystem::path old_path = f.path;
                        std::filesystem::path new_path = old_path.parent_path() / ev2.new_name;
                        std::error_code ec;
                        std::filesystem::rename(old_path, new_path, ec);
                        if (callbacks.on_refresh) callbacks.on_refresh();
                    });
                    dialog->run();
                }, false);
            }
        });

        menu->add_separator();

        if (f.path.find(".local/share/Trash") != std::string::npos) {
            if (callbacks.on_restore) {
                auto item_restore = menu->add_item(i18n().tr("core.file_menu.restore"));
                item_restore->when_click.connect([callbacks, f](auto&) { callbacks.on_restore({f.path}); });
            }
        } else {
            if (callbacks.on_trash) {
                auto item_trash = menu->add_item(i18n().tr("core.file_menu.move_to_trash"));
                item_trash->when_click.connect([callbacks, f](auto&) { callbacks.on_trash({f.path}); });
            }
        }

        if (callbacks.on_delete) {
            auto item_delete = menu->add_item(i18n().tr("core.file_menu.delete"));
            item_delete->when_click.connect([callbacks, f](auto&) { callbacks.on_delete({f.path}); });
        }

        menu->add_separator();

        auto item_props = menu->add_item(i18n().tr("core.file_menu.properties"));
        item_props->when_click.connect([f](horizon::MouseButtonEventContext &ev) {
            if (auto app = static_cast<horizon::Widget*>(ev.sender)->application()) {
                app->add_timer(50, [f]() {
                    auto dialog = std::make_unique<PropertiesDialog>(f);
                    dialog->run();
                }, false);
            }
        });

        if (callbacks.on_open_terminal) {
            menu->add_separator();
            auto item_terminal = menu->add_item(i18n().tr("core.file_menu.open_terminal"));
            item_terminal->when_click.connect([callbacks](auto&) { callbacks.on_open_terminal(); });
        }

        return menu;
    }

    std::unique_ptr<Menu> FileContextMenuBuilder::build_empty_space_menu(
        const std::string &current_path,
        bool show_hidden,
        const Callbacks &callbacks)
    {
        auto menu = std::make_unique<Menu>();

        auto item_new = menu->add_item(i18n().tr("core.file_menu.new_folder"));
        item_new->when_click.connect([current_path, callbacks](horizon::MouseButtonEventContext &ev) {
            if (auto app = static_cast<horizon::Widget*>(ev.sender)->application()) {
                app->add_timer(50, [current_path, callbacks]() {
                    auto dialog = std::make_unique<NewFolderDialog>();
                    dialog->when_accepted.connect([current_path, callbacks](auto& ev2) {
                        std::filesystem::path folder_path = std::filesystem::path(current_path) / ev2.folder_name;
                        std::error_code ec;
                        std::filesystem::create_directory(folder_path, ec);
                        if (callbacks.on_refresh) callbacks.on_refresh();
                    });
                    dialog->run();
                }, false);
            }
        });

        menu->add_separator();

        if (callbacks.on_open_terminal) {
            auto item_terminal = menu->add_item(i18n().tr("core.file_menu.open_terminal"));
            item_terminal->when_click.connect([callbacks](auto&) { callbacks.on_open_terminal(); });
            menu->add_separator();
        }

        if (callbacks.on_toggle_hidden) {
            std::string hidden_text = show_hidden
                                          ? i18n().tr("core.file_menu.hide_hidden")
                                          : i18n().tr("core.file_menu.show_hidden");
            auto item_show_hidden = menu->add_item(hidden_text, "Ctrl+H");
            item_show_hidden->when_click.connect([callbacks](auto&) { callbacks.on_toggle_hidden(); });
        }

        return menu;
    }
} // namespace horizon::files
