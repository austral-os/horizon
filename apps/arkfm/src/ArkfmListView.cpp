#include "ArkfmListView.hpp"
#include "ArkfmFileProvider.hpp"
#include "ArkfmView.hpp"
#include "ArkfmWindow.hpp"
#include "dialogs/PropertiesDialog.hpp"
#include "horizon/Logger.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Menu.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon::arkfm
{
    ArkfmListView::ArkfmListView(std::string path)
    {
        m_current_path = path;
        m_fs_model = std::make_unique<arkutils::FileSystemModel>();

        set_width_mode(horizon::TableViewWidthMode::Fill);

        TableColumn<arkutils::FileInfo> col_icon;
        col_icon.id = "icon";
        col_icon.title = "";
        col_icon.width = 40;
        col_icon.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto icon = std::make_unique<Icon>();
            icon->set_icon_size(24);

            icon->set_icon_name(ArkfmFileProvider::get_icon_name(f));
            return icon;
        };

        TableColumn<arkutils::FileInfo> col_name;
        col_name.id = "name";
        col_name.title = "Name";
        col_name.width = 300;
        col_name.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto lbl = std::make_unique<Label>(ArkfmFileProvider::get_display_name(f));
            if (f.type == arkutils::FileType::Directory)
            {
                lbl->set_font_weight(FONT_WEIGHT_BOLD);
            }
            return lbl;
        };

        TableColumn<arkutils::FileInfo> col_type;
        col_type.id = "type";
        col_type.title = "Type";
        col_type.width = 100;
        col_type.cell_factory = [](const arkutils::FileInfo &f)
        {
            return std::make_unique<Label>(f.type == arkutils::FileType::Directory ? "Folder"
                                                                                   : "File");
        };

        TableColumn<arkutils::FileInfo> col_size;
        col_size.id = "size";
        col_size.title = "Size";
        col_size.width = 100;
        col_size.cell_factory = [](const arkutils::FileInfo &f)
        { return std::make_unique<Label>(std::to_string(f.size / 1024) + " KB"); };

        TableColumn<arkutils::FileInfo> col_mod;
        col_mod.id = "modified";
        col_mod.title = "Modified";
        col_mod.width = 200;
        col_mod.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto t = std::chrono::system_clock::to_time_t(f.last_modified);
            char time_str[26] = {0};
            ctime_r(&t, time_str);
            if (time_str[0] != '\0')
            {
                time_str[24] = '\0'; // Remove newline
            }
            return std::make_unique<Label>(std::string(time_str));
        };

        add_column(col_icon);
        add_column(col_name);
        add_column(col_type);
        add_column(col_size);
        add_column(col_mod);

        // Handle row selection
        // We don't navigate here anymore, parent ArkfmView handles this via the same signal.
        // We keep it empty or remove it if not needed, but signal is public so others can use it.

        m_fs_model->signal_manager().connect(arkutils::FileSystemModel::SIGNAL_DIRECTORY_CHANGED,
                                             [this](SignalContext &ctx)
                                             {
                                                 std::string *path = (std::string *)ctx.data;
                                                 if (path && *path == m_current_path)
                                                 {
                                                     this->refresh(*path);
                                                 }
                                             });

        // Dynamically update background menu before showing
        when_right_click.connect(
            [this](horizon::MouseButtonEventContext &ctx)
            {
                auto bg_menu = std::make_unique<horizon::Menu>();
                bg_menu->set_title("Carpeta");

                ArkfmWindow *win = nullptr;
                if (auto *app = application())
                {
                    win = dynamic_cast<ArkfmWindow *>(app->root());
                }

                auto rename_item = bg_menu->add_item("Cambiar nombre");
                rename_item->set_enabled(false);

                bg_menu->add_separator();

                auto delete_item = bg_menu->add_item("Eliminar");
                delete_item->set_enabled(false);

                bg_menu->add_separator();

                auto bg_prop_item = bg_menu->add_item("Propiedades");
                bg_prop_item->when_click.connect(
                    [this](horizon::MouseButtonEventContext &)
                    {
                        arkutils::FileInfo f;
                        f.name = std::filesystem::path(m_current_path).filename().string();
                        if (f.name.empty())
                            f.name = "/";
                        f.path = m_current_path;
                        f.type = arkutils::FileType::Directory;
                        auto dialog = std::make_unique<PropertiesDialog>(f);
                        dialog->run();
                    });

                set_context_menu(std::move(bg_menu));
            });

        when_application_load.connect([this](EventContext &) { this->refresh(m_current_path); });

        set_row_menu_factory(
            [this](const arkutils::FileInfo &f)
            {
                auto menu = std::make_unique<horizon::Menu>();
                menu->set_title(ArkfmFileProvider::get_display_name(f));

                ArkfmWindow *win = nullptr;
                if (auto *app = application())
                {
                    win = dynamic_cast<ArkfmWindow *>(app->root());
                }

                auto open_item = menu->add_item("Abrir");
                open_item->when_click.connect(
                    [this](horizon::MouseButtonEventContext &)
                    {
                        if (auto *view = dynamic_cast<ArkfmView *>(parent()))
                        {
                            view->open_selection();
                        }
                    });

                auto rename_item = menu->add_item("Cambiar nombre");
                rename_item->when_click.connect(
                    [win, f](horizon::MouseButtonEventContext &)
                    {
                        if (win)
                            win->handle_rename(f.path);
                    });

                menu->add_separator();

                auto delete_item = menu->add_item("Eliminar");
                delete_item->when_click.connect(
                    [win, f](horizon::MouseButtonEventContext &)
                    {
                        if (win)
                            win->handle_delete(f.path);
                    });

                menu->add_separator();

                auto prop_item = menu->add_item("Propiedades");
                prop_item->when_click.connect(
                    [f](horizon::MouseButtonEventContext &)
                    {
                        auto dialog = std::make_unique<PropertiesDialog>(f);
                        dialog->run();
                    });

                return menu;
            });
    }

    void ArkfmListView::refresh(const std::string &path, const std::string &filter)
    {
        LOG_INFO << "Refreshing list view for path: " << path << " with filter: " << filter;
        try
        {
            auto files = m_fs_model->list_directory(path);
            std::vector<arkutils::FileInfo> visible_files;

            std::string filter_lower = filter;
            std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(),
                           ::tolower);

            for (const auto &f : files)
            {
                if (f.is_hidden)
                    continue;

                if (!filter.empty())
                {
                    std::string display_name = ArkfmFileProvider::get_display_name(f);
                    std::transform(display_name.begin(), display_name.end(), display_name.begin(),
                                   ::tolower);

                    if (display_name.find(filter_lower) == std::string::npos)
                    {
                        continue;
                    }
                }

                visible_files.push_back(f);
            }
            update_table(visible_files);
        }
        catch (std::exception &e)
        {
            LOG_ERROR << "Failed to refresh list view: " << e.what();
        }
    }

    void ArkfmListView::update_table(const std::vector<arkutils::FileInfo> &files)
    {
        try
        {
            LOG_INFO << "Updating table with " << files.size() << " files.";
            // TableView stores data by value, copy it.
            std::vector<arkutils::FileInfo> files_copy = files;
            this->set_data(std::move(files_copy));
        }
        catch (std::exception &e)
        {
            LOG_ERROR << "Failed to update table: " << e.what();
        }
    }

} // namespace horizon::arkfm