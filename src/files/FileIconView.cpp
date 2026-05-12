#include "horizon/files/FileIconView.hpp"
#include "horizon/files/FileIconProvider.hpp"
#include "horizon/Application.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Label.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Menu.hpp"
#include "horizon/ThemeManager.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>
#include <thread>
#include "horizon/arkutils/FileOperations.hpp"

namespace horizon::files
{
    class FileIconItem : public Widget
    {
    public:
        FileIconItem() : Widget()
        {
            auto icon = std::make_unique<Icon>();
            icon->set_position_type(FREE);
            m_icon_ptr = icon.get();
            add_child(std::move(icon));

            auto label = std::make_unique<Label>();
            label->set_position_type(FREE);
            label->set_alignment(TextAlignment::Center);
            label->set_vertical_alignment(VerticalAlignment::Middle);
            m_label_ptr = label.get();
            add_child(std::move(label));

            m_position_type = FREE;
            
            set_draggable(true);
            when_drag_start.connect([this](DragEventContext &ctx) {
                if (application()) {
                    std::vector<std::string> mimes = {"text/uri-list", "text/plain"};
                    application()->start_drag(mimes, [this](const std::string &mime) -> std::vector<uint8_t> {
                        if (mime == "text/uri-list") {
                            std::string uri = "file://" + m_file_info.path + "\r\n";
                            return std::vector<uint8_t>(uri.begin(), uri.end());
                        }
                        return std::vector<uint8_t>(m_file_info.path.begin(), m_file_info.path.end());
                    }, this);
                }
            });

            when_drop.connect([this](DropEventContext &ctx) {
                auto data = ctx.get_data("text/uri-list");
                if (data.empty()) return;

                std::string uris(data.begin(), data.end());
                
                // Simple parsing of text/uri-list
                size_t start = 0;
                while (start < uris.length()) {
                    size_t pos = uris.find("file://", start);
                    if (pos == std::string::npos) break;
                    
                    size_t end = uris.find("\r\n", pos);
                    std::string src = uris.substr(pos + 7, (end == std::string::npos) ? std::string::npos : end - (pos + 7));
                    
                    if (!src.empty()) {
                        std::filesystem::path p(src);
                        std::string dest = m_file_info.path + "/" + p.filename().string();
                        
                        if (src != dest) {
                            auto future = arkutils::FileOperations::copy(src, dest);
                            std::thread([f = std::move(future)]() mutable {
                                f.get();
                            }).detach();
                        }
                    }
                    
                    if (end == std::string::npos) break;
                    start = end + 2;
                }
            });
        }

        void set_data(const arkutils::FileInfo &f, float zoom, bool selected)
        {
            m_file_info = f;
            m_zoom = zoom;
            m_selected = selected;
            m_label_ptr->set_text(FileIconProvider::get_display_name(f));

            std::string icon_name = FileIconProvider::get_icon_name(f);

            m_icon_size = static_cast<int>(48 * m_zoom);
            m_icon_ptr->set_icon_name(icon_name);
            m_icon_ptr->set_icon_size(m_icon_size);

            m_label_ptr->set_font_size(10 * m_zoom);

            if (f.type == arkutils::FileType::Directory)
            {
                set_accept_drops(true);
            }
            else
            {
                set_accept_drops(false);
            }

            invalidate();
        }

        int preferred_height(int width) const override
        {
            int padding = static_cast<int>(4 * m_zoom);
            int gap = 4;
            int label_h = m_label_ptr->preferred_height(width - 4);
            return padding + m_icon_size + gap + label_h + padding;
        }

        void calculate_layout() override
        {
            int padding = static_cast<int>(4 * m_zoom);
            int icon_y = padding;

            m_icon_ptr->set_position(m_x + (m_width - m_icon_size) / 2, m_y + icon_y);
            m_icon_ptr->set_size(m_icon_size, m_icon_size);

            int label_y = icon_y + m_icon_size + 4;
            m_label_ptr->set_position(m_x + 2, m_y + label_y);
            m_label_ptr->set_size(m_width - 4, m_height - label_y - 2);
        }

        void draw(GraphicsContext &gc) override
        {
            if (m_selected)
            {
                auto *tm = application()->theme_manager.get();
                Color bg = tm->get_color("table_row_selected");
                Color fg = tm->get_color("table_row_selected_fg");

                int padding = static_cast<int>(4 * m_zoom);
                int label_y = padding + m_icon_size + 4;

                int h_x = m_x + 2;
                int h_y = m_y + label_y;
                int h_w = m_width - 4;
                int h_h = m_height - label_y - 2;

                gc.setColor(bg);
                gc.fillRect(h_x, h_y, h_w, h_h, CornerRadius(6));

                m_label_ptr->set_text_color(fg);
            }
            else
            {
                m_label_ptr->set_text_color(Color(0.0f, 0.0f, 0.0f, 1.0f));
            }
        }

    private:
        Icon *m_icon_ptr{nullptr};
        Label *m_label_ptr{nullptr};
        arkutils::FileInfo m_file_info;
        float m_zoom{1.5f};
        int m_icon_size{48};
        bool m_selected{false};
    };

    FileIconView::FileIconView(std::string path) : IconView<arkutils::FileInfo>()
    {
        set_position_type(FILL);
        set_zoom(1.5f);
        set_focusable(true);
        m_current_path = std::move(path);
        m_fs_model = std::make_unique<arkutils::FileSystemModel>();

        set_item_size(120, 130);
        set_side_margin(20);

        set_item_factory(
            [this](const arkutils::FileInfo &f, float zoom, bool selected)
            {
                auto item = std::make_unique<FileIconItem>();
                item->set_data(f, zoom, selected);
                return item;
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


    }

    void FileIconView::set_application_recursive(WaylandWindow *app)
    {
        IconView<arkutils::FileInfo>::set_application_recursive(app);
        refresh(m_current_path);
    }

    void FileIconView::refresh(const std::string &path, const std::string &filter)
    {
        if (application() && application()->w_surface())
            application()->w_surface()->set_cursor(CursorType::Wait);

        LOG_INFO << "Refreshing icon view for path: " << path << " with filter: " << filter;
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
            update_icons(visible_files);
        }
        catch (std::exception &e)
        {
            LOG_ERROR << "Failed to refresh icon view: " << e.what();
        }

        if (application() && application()->w_surface())
            application()->w_surface()->set_cursor(CursorType::Default);
    }

    void FileIconView::update_icons(const std::vector<arkutils::FileInfo> &files)
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

        LOG_INFO << "FileIconView [" << (void *)this << "]: Updating icons with " << unique_files.size()
                 << " unique items (discarded " << (files.size() - unique_files.size()) << "). Path: " << m_current_path;

        set_data(unique_files);
    }
} // namespace horizon::files
