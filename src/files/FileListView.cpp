#include "horizon/files/FileListView.hpp"
#include "horizon/Logger.hpp"
#include "horizon/arkutils/FileOperations.hpp"
#include "horizon/files/FileIconProvider.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <horizon/Application.hpp>
#include <horizon/FormatUtils.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Menu.hpp>
#include <set>
#include <thread>

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
            lbl->set_editable(true);
            lbl->when_text_edited.connect([f, label_ptr = lbl.get()](const EventContext&) {
                std::string new_name = label_ptr->text();
                if (new_name != FileIconProvider::get_display_name(f) && !new_name.empty()) {
                    std::filesystem::path p(f.path);
                    std::string new_path = p.parent_path() / new_name;
                    arkutils::FileOperations::rename(f.path, new_path);
                }
            });
            lbl->set_font_size(14);
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
            auto lbl = std::make_unique<Label>(f.type == arkutils::FileType::Directory ? "Folder"
                                                                                       : "File");
            lbl->set_font_size(14);
            return lbl;
        };

        TableColumn<arkutils::FileInfo> col_size;
        col_size.id = "size";
        col_size.title = "Size";
        col_size.width = 100;
        col_size.cell_factory = [](const arkutils::FileInfo &f)
        {
            if (f.type == arkutils::FileType::Directory)
            {
                auto lbl = std::make_unique<Label>("---");
                lbl->set_font_size(11);
                return lbl;
            }
            auto lbl = std::make_unique<Label>(horizon::format_bytes(f.size));
            lbl->set_font_size(11);
            return lbl;
        };

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
            auto lbl = std::make_unique<Label>(std::string(time_str));
            lbl->set_font_size(11);
            return lbl;
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
                                                     if (application()) {
                                                         std::string p = *path;
                                                         application()->post_task([this, p]() {
                                                             this->refresh(p);
                                                         });
                                                     } else {
                                                         this->refresh(*path);
                                                     }
                                                 }
                                             });

        set_row_setup_callback(
            [this](TableRow *row, const arkutils::FileInfo &f)
            {
                row->set_draggable(true);
                row->when_drag_start.connect(
                    [this, f, row](DragEventContext &ctx)
                    {
                        if (application())
                        {
                            std::vector<std::string> mimes = {"text/uri-list", "text/plain"};
                            application()->start_drag(
                                mimes,
                                [f](const std::string &mime) -> std::vector<uint8_t>
                                {
                                    if (mime == "text/uri-list")
                                    {
                                        std::string uri = "file://" + f.path + "\r\n";
                                        return std::vector<uint8_t>(uri.begin(), uri.end());
                                    }
                                    return std::vector<uint8_t>(f.path.begin(), f.path.end());
                                },
                                row);
                        }
                    });

                if (f.type == arkutils::FileType::Directory)
                {
                    row->set_accept_drops(true);
                    row->when_drop.connect(
                        [this, f](DropEventContext &ctx)
                        {
                            auto data = ctx.get_data("text/uri-list");
                            if (data.empty())
                                return;

                            std::string uris(data.begin(), data.end());

                            size_t start = 0;
                            while (start < uris.length())
                            {
                                size_t pos = uris.find("file://", start);
                                if (pos == std::string::npos)
                                    break;

                                size_t end = uris.find("\r\n", pos);
                                std::string src = uris.substr(pos + 7, (end == std::string::npos)
                                                                           ? std::string::npos
                                                                           : end - (pos + 7));

                                if (!src.empty())
                                {
                                    std::filesystem::path p(src);
                                    std::filesystem::path dst_dir(f.path);
                                    std::filesystem::path dest = dst_dir / p.filename();
                                    
                                    if (std::filesystem::exists(dest)) {
                                        std::string base = p.stem().string();
                                        std::string ext = p.extension().string();
                                        dest = dst_dir / ("Copia de " + base + ext);
                                        int counter = 1;
                                        while (std::filesystem::exists(dest)) {
                                            dest = dst_dir / ("Copia de " + base + " " + std::to_string(counter) + ext);
                                            counter++;
                                        }
                                    }

                                    if (src != dest.string())
                                    {
                                        auto future = arkutils::FileOperations::copy(
                                            src, dest.string(),
                                            [this](double progress)
                                            {
                                                if (application())
                                                {
                                                    application()->post_task(
                                                        [this, progress]()
                                                        {
                                                            OperationProgressEvent ev;
                                                            ev.progress = progress;
                                                            ev.finished = (progress >= 1.0);
                                                            this->when_operation_progress.run(ev);
                                                        });
                                                }
                                            });

                                        std::thread(
                                            [f_future = std::move(future), this]() mutable
                                            {
                                                auto result = f_future.get();
                                                if (application())
                                                {
                                                    application()->post_task(
                                                        [this, result]()
                                                        {
                                                            if (result == arkutils::FileOperations::
                                                                              Result::Success)
                                                            {
                                                                OperationProgressEvent ev;
                                                                ev.progress = 1.0;
                                                                ev.finished = true;
                                                                this->when_operation_progress.run(
                                                                    ev);
                                                            }
                                                        });
                                                }
                                            })
                                            .detach();
                                    }
                                }

                                if (end == std::string::npos)
                                    break;
                                start = end + 2;
                            }
                        });
                }
            });

        when_key_release.connect([this](KeyEventContext &ev) {
            if (ev.keysym == XKB_KEY_F2) {
                int idx = this->selected_index();
                if (idx >= 0 && this->children().size() > 1) {
                    auto* scroll = static_cast<ScrollArea*>(this->children()[1].get());
                    if (!scroll->children().empty()) {
                        auto* content = scroll->children()[0].get();
                        if (idx < (int)content->children().size()) {
                            auto* row = content->children()[idx].get();
                            if (row && row->children().size() > 1) {
                                auto* label = dynamic_cast<Label*>(row->children()[1].get());
                                if (label && label->is_editable()) {
                                    label->begin_edit();
                                    ev.stop_propagation = true;
                                }
                            }
                        }
                    }
                }
            }
        });
    }

    void FileListView::set_application_recursive(WaylandWindow *app)
    {
        // Call the base first (sets up header/content containers)
        TableView<arkutils::FileInfo>::set_application_recursive(app);
        // Do a single, controlled load — no timer, no double refresh
        refresh(m_current_path);
    }

    void FileListView::refresh(const std::string &path, const std::string &filter)
    {
        if (application() && application()->w_surface())
            application()->w_surface()->set_cursor(CursorType::Wait);

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
                if (f.is_hidden && !m_show_hidden_files)
                    continue;

                // Apply file filter for non-directories
                if (f.type != arkutils::FileType::Directory && !m_file_filter.empty()) {
                    bool matched = false;
                    for (const auto& pat : m_file_filter) {
                        if (pat == "*" || pat == "*.*") {
                            matched = true;
                            break;
                        }
                        
                        // Handle simple extension matching like "*.png" or "image/png" -> .png
                        std::string ext = pat;
                        if (ext.find("*.") == 0) ext = ext.substr(1); // keeps ".png"
                        // we also might receive just ".png"
                        if (f.name.length() >= ext.length() && 
                            f.name.compare(f.name.length() - ext.length(), ext.length(), ext) == 0) {
                            matched = true;
                            break;
                        }
                    }
                    if (!matched) continue;
                }

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

        if (application() && application()->w_surface())
            application()->w_surface()->set_cursor(CursorType::Default);
    }

    void FileListView::update_table(const std::vector<arkutils::FileInfo> &files)
    {
        try
        {
            std::vector<arkutils::FileInfo> unique_files;
            std::set<std::string> seen_paths;

            for (const auto &f : files)
            {
                if (seen_paths.find(f.path) == seen_paths.end())
                {
                    unique_files.push_back(f);
                    seen_paths.insert(f.path);
                }
            }

            LOG_INFO << "FileListView [" << (void *)this << "]: Updating table with "
                     << unique_files.size() << " unique items (discarded "
                     << (files.size() - unique_files.size()) << "). Path: " << m_current_path;

            this->set_data(std::move(unique_files));
        }
        catch (std::exception &e)
        {
            LOG_ERROR << "Failed to update table: " << e.what();
        }
    }

} // namespace horizon::files
