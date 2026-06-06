#include "horizon/files/FileIconView.hpp"
#include "horizon/files/FileView.hpp"
#include "horizon/files/FileIconProvider.hpp"
#include "horizon/Application.hpp"
#include "horizon/Icon.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include "horizon/IconThemeLookup.hpp"
#include "horizon/Label.hpp"
#include "horizon/lens/ThumbnailCache.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Menu.hpp"
#include "horizon/ThemeManager.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <random>
#include <set>
#include <thread>
#include "horizon/arkutils/FileOperations.hpp"

namespace fs = std::filesystem;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace horizon::files
{
    // -----------------------------------------------------------------------
    // FolderPreviewWidget — composites a folder icon with up to 4 rotated
    // thumbnail previews of images inside the folder.
    // -----------------------------------------------------------------------
    class FolderPreviewWidget : public Widget
    {
    public:
        FolderPreviewWidget() : Widget() {}

        void set_folder(const std::string &folder_path, int icon_size)
        {
            m_icon_size = icon_size;

            if (m_folder_path != folder_path)
            {
                m_folder_path = folder_path;
                m_folder_icon.clear();
                m_thumbnails.clear();
                scan_images();
            }

            if (m_folder_icon.empty())
            {
                m_folder_icon = IconThemeLookup::find_icon("folder", m_icon_size);
            }

            invalidate();
        }

    protected:
        void draw(GraphicsContext &ctx) override
        {
            // Center the folder icon in our area
            int icon_x = m_x + (m_width - m_icon_size) / 2;
            int icon_y = m_y + (m_height - m_icon_size) / 2;

            // Draw folder icon as background
            if (!m_folder_icon.empty())
            {
                ctx.drawImage(m_folder_icon, icon_x, icon_y, m_icon_size, m_icon_size, 1.0f);
            }

            // Draw thumbnail overlays on top
            draw_thumbnails(ctx, icon_x, icon_y);
        }

    private:
        void scan_images()
        {
            m_thumbnails.clear();
            m_angles.clear();
            
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(-20.0f, 20.0f);

            try
            {
                int count = 0;
                for (const auto &entry : fs::directory_iterator(m_folder_path))
                {
                    if (!entry.is_regular_file())
                        continue;
                    if (count >= 4)
                        break;

                    std::string entry_path = entry.path().string();
                    if (!lens::ThumbnailCache::is_supported(entry_path))
                        continue;

                    // Try Normal first (128×128, ideal size for small previews),
                    // fall back to Large (256×256) if Normal not available yet.
                    std::string thumb = lens::ThumbnailCache::get_thumbnail(
                        entry_path, lens::ThumbnailSize::Normal);
                    if (thumb.empty())
                    {
                        thumb = lens::ThumbnailCache::get_thumbnail(
                            entry_path, lens::ThumbnailSize::Large);
                    }

                    if (!thumb.empty())
                    {
                        m_thumbnails.push_back(thumb);
                        m_angles.push_back(dis(gen));
                        count++;
                    }
                    else
                    {
                        lens::ThumbnailCache::request_thumbnail(
                            entry_path, lens::ThumbnailSize::Normal);
                    }
                }
            }
            catch (...)
            {
                // Permission denied, broken symlink, etc. — silently skip
            }
        }

        void draw_thumbnails(GraphicsContext &ctx, int icon_x, int icon_y)
        {
            int n = (int)m_thumbnails.size();
            if (n == 0)
                return;

            int cx = icon_x + m_icon_size / 2;
            int cy = icon_y + m_icon_size / 2;
            float alpha = 0.92f;
            
            int base_thumb_size = std::max(8, (int)(m_icon_size * 0.40f));
            int single_thumb_size = std::max(12, (int)(m_icon_size * 0.60f));

            for (int i = 0; i < n && i < (int)m_thumbnails.size(); i++)
            {
                ctx.save();
                ctx.translate((float)cx, (float)cy);
                ctx.rotate(m_angles[i] * (float)(M_PI / 180.0f));
                
                if (n == 1) {
                    ctx.drawImage(m_thumbnails[i], -single_thumb_size / 2, -single_thumb_size / 2,
                                  single_thumb_size, single_thumb_size, alpha);
                } else if (n == 2) {
                    int w = base_thumb_size;
                    int h = single_thumb_size;
                    int gap = 3;
                    int x_offset = (i == 0) ? -w - gap : gap;
                    int y_offset = -h / 2;
                    ctx.drawImage(m_thumbnails[i], x_offset, y_offset, w, h, alpha);
                } else {
                    int thumb_size = std::max(5, base_thumb_size - 3);
                    int x_offset = 0;
                    int y_offset = 0;
                    int gap = 3;
                    
                    int overlap_y = thumb_size / 4;
                    
                    if (i == 0) { // Top Left
                        x_offset = -thumb_size - gap;
                        y_offset = -thumb_size + overlap_y - gap;
                    } else if (i == 1) { // Top Right
                        x_offset = gap;
                        y_offset = -thumb_size + overlap_y - gap;
                    } else if (i == 2) { // Bottom Left
                        x_offset = -thumb_size - gap;
                        y_offset = -overlap_y + gap;
                    } else if (i == 3) { // Bottom Right
                        x_offset = gap;
                        y_offset = -overlap_y + gap;
                    }
                    
                    ctx.drawImage(m_thumbnails[i], x_offset, y_offset,
                                  thumb_size, thumb_size, alpha);
                }
                ctx.restore();
            }
        }

        std::string m_folder_path;
        std::string m_folder_icon;
        int m_icon_size{48};
        std::vector<std::string> m_thumbnails;
        std::vector<float> m_angles;
    };

    class FileIconItem : public Widget
    {
    public:
        FileIconItem() : Widget()
        {
            auto icon = std::make_unique<Icon>();
            icon->set_position_type(FREE);
            m_icon_ptr = icon.get();
            add_child(std::move(icon));

            auto preview = std::make_unique<FolderPreviewWidget>();
            preview->set_position_type(FREE);
            m_preview_ptr = preview.get();
            add_child(std::move(preview));
            m_preview_ptr->set_visible(false);

            auto label = std::make_unique<Label>();
            label->set_position_type(FREE);
            label->set_alignment(TextAlignment::Center);
            label->set_vertical_alignment(VerticalAlignment::Middle);
            label->set_editable(true);
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
                        std::filesystem::path dst_dir(m_file_info.path);
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
                        
                        if (src != dest.string()) {
                            auto future = arkutils::FileOperations::copy(src, dest.string());
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

            m_label_ptr->when_text_edited.disconnect_all();
            m_label_ptr->when_text_edited.connect([this, f](const EventContext&) {
                std::string new_name = m_label_ptr->text();
                if (new_name != FileIconProvider::get_display_name(f) && !new_name.empty()) {
                    std::filesystem::path p(f.path);
                    std::string new_path = p.parent_path() / new_name;
                    arkutils::FileOperations::rename(f.path, new_path);
                }
            });

            m_icon_size = static_cast<int>(48 * m_zoom);

            if (f.type == arkutils::FileType::Directory)
            {
                set_accept_drops(true);
                m_icon_ptr->set_visible(false);
                m_preview_ptr->set_visible(true);
                m_preview_ptr->set_folder(f.path, m_icon_size);
            }
            else
            {
                set_accept_drops(false);
                m_icon_ptr->set_visible(true);
                m_preview_ptr->set_visible(false);

                // Use cached thumbnail if available, otherwise request generation
                std::string thumb_path = lens::ThumbnailCache::get_thumbnail(f.path,
                    lens::ThumbnailSize::Large);
                if (!thumb_path.empty())
                {
                    m_icon_ptr->set_icon_path(thumb_path);
                }
                else
                {
                    std::string icon_name = FileIconProvider::get_icon_name(f);
                    m_icon_ptr->set_icon_name(icon_name);

                    if (lens::ThumbnailCache::is_supported(f.path))
                    {
                        lens::ThumbnailCache::request_thumbnail(f.path,
                            lens::ThumbnailSize::Large);
                    }
                }

                m_icon_ptr->set_icon_size(m_icon_size);
            }

            m_label_ptr->set_font_size(10 * m_zoom);

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
            int icon_x = m_x + (m_width - m_icon_size) / 2;

            m_icon_ptr->set_position(icon_x, m_y + icon_y);
            m_icon_ptr->set_size(m_icon_size, m_icon_size);

            m_preview_ptr->set_position(icon_x, m_y + icon_y);
            m_preview_ptr->set_size(m_icon_size, m_icon_size);

            int label_y = icon_y + m_icon_size + 4;
            m_label_ptr->set_position(m_x + 2, m_y + label_y);
            m_label_ptr->set_size(m_width - 4, m_height - label_y - 2);
        }

        void draw(GraphicsContext &gc) override
        {
            auto *tm = theme_manager();

            if (m_selected)
            {
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
                m_label_ptr->set_text_color(m_default_text_color);
            }
        }

        void set_default_text_color(Color c) { m_default_text_color = c; }
        
        void enable_desktop_mode()
        {
            m_label_ptr->set_has_shadow(true);
            m_label_ptr->set_text_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
            set_default_text_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
        }

        void begin_rename()
        {
            if (m_label_ptr && m_label_ptr->is_editable()) {
                m_label_ptr->begin_edit();
            }
        }

    private:
        Icon *m_icon_ptr{nullptr};
        FolderPreviewWidget *m_preview_ptr{nullptr};
        Label *m_label_ptr{nullptr};
        arkutils::FileInfo m_file_info;
        float m_zoom{1.5f};
        int m_icon_size{48};
        bool m_selected{false};
        Color m_default_text_color{0.0f, 0.0f, 0.0f, -1.0f};
    };

    FileIconView::~FileIconView()
    {
        if (m_thumbnail_timer_id != 0 && application())
        {
            application()->stop_timer(m_thumbnail_timer_id);
            m_thumbnail_timer_id = 0;
        }
    }

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
                if (m_transparent) {
                    item->enable_desktop_mode();
                }
                item->set_data(f, zoom, selected);
                auto* item_ptr = item.get();
                item_ptr->when_mouse_press.connect([this](MouseButtonEventContext& ctx) {
                    this->set_focus(true);
                });
                return item;
            });

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



        when_key_release.connect([this](KeyEventContext &ev) {
            if (ev.keysym == XKB_KEY_F2) {
                int idx = selected_index();
                if (idx >= 0 && m_content_pane && idx < (int)m_content_pane->children().size()) {
                    auto* item = dynamic_cast<FileIconItem*>(m_content_pane->children()[idx].get());
                    if (item) {
                        item->begin_rename();
                        ev.stop_propagation = true;
                    }
                }
            }
        });

        // Ensure we grab focus when clicked in empty space
        m_scroll_area->when_mouse_press.connect([this](MouseButtonEventContext &ctx) {
            set_focus(true);
            if (ctx.button == 0x110 || ctx.button == 0x111)
            {
                clear_selection();
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
                        
                        std::string ext = pat;
                        if (ext.find("*.") == 0) ext = ext.substr(1);
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
        start_thumbnail_watch();
    }

    void FileIconView::start_thumbnail_watch()
    {
        if (!application())
            return;

        // Cancel any existing watch timer
        if (m_thumbnail_timer_id != 0)
        {
            application()->stop_timer(m_thumbnail_timer_id);
            m_thumbnail_timer_id = 0;
        }

        // Check if any files in the current view could use a thumbnail
        for (const auto &f : data())
        {
            if (lens::ThumbnailCache::is_supported(f.path) &&
                lens::ThumbnailCache::get_thumbnail(f.path, lens::ThumbnailSize::Large).empty())
            {
                m_thumbnail_timer_id = application()->add_timer(3000,
                    [this]()
                    {
                        m_thumbnail_timer_id = 0;
                        check_thumbnails();
                    });
                break;
            }
        }
    }

    void FileIconView::check_thumbnails()
    {
        if (!application())
            return;

        // Look for any file that now has a valid thumbnail
        bool has_new = false;
        for (const auto &f : data())
        {
            if (!lens::ThumbnailCache::is_supported(f.path))
                continue;

            if (!lens::ThumbnailCache::get_thumbnail(f.path, lens::ThumbnailSize::Large).empty())
            {
                has_new = true;
                break;
            }
        }

        if (has_new)
        {
            rebuild_items();
            return;
        }

        // Still pending — reschedule
        bool any_pending = false;
        for (const auto &f : data())
        {
            if (lens::ThumbnailCache::is_supported(f.path) &&
                lens::ThumbnailCache::get_thumbnail(f.path, lens::ThumbnailSize::Large).empty())
            {
                any_pending = true;
                break;
            }
        }

        if (any_pending)
        {
            m_thumbnail_timer_id = application()->add_timer(3000,
                [this]()
                {
                    m_thumbnail_timer_id = 0;
                    check_thumbnails();
                });
        }
    }

    bool FileIconView::supports_clipboard() const
    {
        if (parent() && dynamic_cast<FileView*>(parent())) return false;
        return true;
    }

    bool FileIconView::can_perform(ClipboardAction action) const
    {
        if (action == ClipboardAction::Copy || action == ClipboardAction::Cut)
            return !get_selected_items().empty();
        return (action == ClipboardAction::Paste);
    }

    void FileIconView::perform(ClipboardAction action)
    {
        LOG_INFO << "[FileIconView] perform clipboard action: " << (int)action;
        if (action == ClipboardAction::Copy || action == ClipboardAction::Cut)
        {
            auto selection = get_selected_items();
            m_clipboard_paths.clear();
            for (const auto &item : selection) m_clipboard_paths.push_back(item.path);
            m_is_cut = (action == ClipboardAction::Cut);

            if (application()) {
                LOG_INFO << "[FileIconView] application() is valid. Calling set_clipboard_owner on " << (void*)application();
                application()->set_clipboard_owner(this);
                LOG_INFO << "[FileIconView] set_clipboard_owner finished.";
            } else {
                LOG_INFO << "[FileIconView] application() is NULL!";
            }
        }
        else if (action == ClipboardAction::Paste)
        {
            if (application()) {
                LOG_INFO << "[FileIconView] calling request_clipboard_data on " << (void*)application();
                application()->request_clipboard_data(this, "text/uri-list");
            } else {
                LOG_INFO << "[FileIconView] application() is NULL!";
            }
        }
    }

    std::vector<std::string> FileIconView::provided_mime_types() const
    {
        return {"text/uri-list"};
    }

    std::vector<std::string> FileIconView::accepted_mime_types() const
    {
        return {"text/uri-list"};
    }

    void FileIconView::provide_clipboard_data(const std::string &mime, DataSink &sink)
    {
        LOG_INFO << "[FileIconView] provide_clipboard_data: " << mime << " paths: " << m_clipboard_paths.size();
        if (mime == "text/uri-list")
        {
            std::string data;
            for (const auto &path : m_clipboard_paths) data += "file://" + path + "\r\n";
            sink.write(std::vector<uint8_t>(data.begin(), data.end()));
            sink.done();
        }
        else sink.error();
    }

    void FileIconView::on_clipboard_data_received(const std::string &mime, const std::vector<uint8_t> &data)
    {
        LOG_INFO << "[FileIconView] on_clipboard_data_received: " << mime << " data size: " << data.size();
        if (mime != "text/uri-list" || data.empty()) return;

        std::string content(data.begin(), data.end());
        std::stringstream ss(content);
        std::string line;
        std::vector<std::string> paths;

        while (std::getline(ss, line))
        {
            if (line.empty()) continue;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.find("file://") == 0) paths.push_back(line.substr(7));
        }

        if (paths.empty()) return;

        for (const auto &src_path : paths)
        {
            std::filesystem::path src(src_path);
            std::filesystem::path dst_dir(m_current_path);
            std::filesystem::path dst = dst_dir / src.filename();
            
            if (src == dst_dir || dst_dir.string().find(src.string() + "/") == 0) continue;

            if (std::filesystem::exists(dst)) {
                if (src == dst && m_is_cut) continue;
                
                std::string base = src.stem().string();
                std::string ext = src.extension().string();
                dst = dst_dir / ("Copia de " + base + ext);
                int counter = 1;
                while (std::filesystem::exists(dst)) {
                    dst = dst_dir / ("Copia de " + base + " " + std::to_string(counter) + ext);
                    counter++;
                }
            }
            
            if (m_is_cut) {
                auto future = arkutils::FileOperations::move(src_path, dst.string());
                std::thread([this, f = std::move(future)]() mutable {
                    f.get();
                    if (application()) {
                        application()->post_task([this]() {
                            this->refresh(m_current_path);
                        });
                    }
                }).detach();
            } else {
                auto future = arkutils::FileOperations::copy(src_path, dst.string(), nullptr);
                std::thread([this, f = std::move(future)]() mutable {
                    f.get();
                    if (application()) {
                        application()->post_task([this]() {
                            this->refresh(m_current_path);
                        });
                    }
                }).detach();
            }
        }
    }

} // namespace horizon::files
