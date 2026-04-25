#include "horizon/files/FileListView.hpp"
#include "horizon/files/FileIconProvider.hpp"
#include "horizon/Logger.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Menu.hpp>

namespace horizon::files
{
    FileListView::FileListView(std::string path)
    {
        m_current_path = path;
        m_fs_model = std::make_unique<arkutils::FileSystemModel>();

        set_width_mode(horizon::TableViewWidthMode::Fill);
        set_focusable(true);

        TableColumn<arkutils::FileInfo> col_icon;
        col_icon.id = "icon";
        col_icon.title = "";
        col_icon.width = 40;
        col_icon.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto icon = std::make_unique<Icon>();
            icon->set_icon_size(24);
            icon->set_icon_name(FileIconProvider::get_icon_name(f));
            return icon;
        };

        TableColumn<arkutils::FileInfo> col_name;
        col_name.id = "name";
        col_name.title = "Name";
        col_name.width = 300;
        col_name.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto lbl = std::make_unique<Label>(FileIconProvider::get_display_name(f));
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

        m_fs_model->signal_manager().connect(arkutils::FileSystemModel::SIGNAL_DIRECTORY_CHANGED,
                                             [this](SignalContext &ctx)
                                             {
                                                 std::string *path = (std::string *)ctx.data;
                                                 if (path && *path == m_current_path)
                                                 {
                                                     this->refresh(*path);
                                                 }
                                             });

        when_application_load.connect([this](EventContext &) { this->refresh(m_current_path); });
    }

    void FileListView::refresh(const std::string &path, const std::string &filter)
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
                    std::string display_name = FileIconProvider::get_display_name(f);
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

    void FileListView::update_table(const std::vector<arkutils::FileInfo> &files)
    {
        try
        {
            LOG_INFO << "Updating table with " << files.size() << " files.";
            std::vector<arkutils::FileInfo> files_copy = files;
            this->set_data(std::move(files_copy));
        }
        catch (std::exception &e)
        {
            LOG_ERROR << "Failed to update table: " << e.what();
        }
    }

} // namespace horizon::files
