#include "ArkfmListView.hpp"
#include "horizon/Logger.hpp"
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
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

            std::string icon_name = "text-x-generic";
            if (f.type == arkutils::FileType::Directory)
            {
                icon_name = "folder";
            }
            else if (f.extension == "png" || f.extension == "jpg" || f.extension == "jpeg" ||
                     f.extension == "svg")
            {
                icon_name = "image-x-generic";
            }
            else if (f.extension == "cpp" || f.extension == "hpp" || f.extension == "c" ||
                     f.extension == "h")
            {
                icon_name = "text-x-csrc";
            }
            else if (f.extension == "txt" || f.extension == "md")
            {
                icon_name = "text-x-generic";
            }
            else if (f.extension == "pdf")
            {
                icon_name = "document-pdf";
            }
            else if (f.extension == "zip" || f.extension == "tar" || f.extension == "gz")
            {
                icon_name = "package-x-generic";
            }

            icon->set_icon_name(icon_name);
            return icon;
        };

        TableColumn<arkutils::FileInfo> col_name;
        col_name.id = "name";
        col_name.title = "Name";
        col_name.width = 300;
        col_name.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto lbl = std::make_unique<Label>(f.name);
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
        when_row_dbl_click.connect(
            [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
            {
                if (ctx.row_data.type == arkutils::FileType::Directory)
                {
                    m_current_path = ctx.row_data.path;
                    m_fs_model->unwatch_all(); // Important: we should stop watching old folders if
                                               // we only care about the new one, but for now we
                                               // just change path.
                    this->refresh(m_current_path);

                    // Update window title or add a label if desired
                    LOG_INFO << "Navigating to: " << m_current_path;
                }
            });

        m_fs_model->signal_manager().connect(arkutils::FileSystemModel::SIGNAL_DIRECTORY_CHANGED,
                                             [this](SignalContext &ctx)
                                             {
                                                 std::string *path = (std::string *)ctx.data;
                                                 if (path && *path == m_current_path)
                                                 {
                                                     this->refresh(*path);
                                                 }
                                             });

        refresh(m_current_path);
    }

    void ArkfmListView::refresh(const std::string &path)
    {
        LOG_INFO << "Refreshing list view for path: " << path;
        try
        {
            auto files = m_fs_model->list_directory(path);
            std::vector<arkutils::FileInfo> visible_files;
            for (const auto &f : files)
            {
                if (!f.is_hidden)
                {
                    visible_files.push_back(f);
                }
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