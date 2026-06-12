#include <ImagesView.hpp>
#include <algorithm>
#include <filesystem>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Color.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/lens/ThumbnailCache.hpp>

namespace horizon::preferences
{
    const int THUMB_WIDTH = 130;
    const int THUMB_HEIGHT = 86;
    const int GRID_SPACING = 10;
    const int SCROLLBAR_MARGIN = 10;
    const int BOTTOM_PADDING = 20;

    // --- ThumbnailItem ---
    ThumbnailItem::ThumbnailItem(const std::string &original_path,
                                 const std::string &thumbnail_path)
        : m_original_path(original_path)
    {
        set_size(THUMB_WIDTH, THUMB_HEIGHT);
        set_margin(4); // Subtle padding

        auto img = std::make_unique<Image>();
        if (!thumbnail_path.empty()) {
            img->set_path(thumbnail_path);
        }
        img->set_mode(ImageMode::Crop); // Scale to fill the box without empty spaces
        img->set_position_type(horizon::WidgetPositionTypes::FILL);
        m_image = img.get();
        add_child(std::move(img));

        set_cursor_type(CursorType::Pointer);
    }

    void ThumbnailItem::draw(horizon::GraphicsContext &gc)
    {
        horizon::Widget::draw(gc);
    }

    void ThumbnailItem::set_thumbnail(const std::string& thumbnail_path)
    {
        if (m_image) {
            m_image->set_path(thumbnail_path);
            invalidate();
        }
    }

    // --- ThumbnailGrid ---
    class ThumbnailGrid : public horizon::Widget
    {
    public:
        ThumbnailGrid() : horizon::Widget()
        {
            set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        }

        void calculate_layout() override
        {
            int width_avail = width();
            if (width_avail <= 0)
                return;

            int columns = std::max(1, (width_avail + GRID_SPACING) / (THUMB_WIDTH + GRID_SPACING));

            int count = 0;
            for (auto const &child : m_children)
            {
                if (child->position_type() == FREE)
                {
                    int row = count / columns;
                    int col = count % columns;

                    int rel_x = col * (THUMB_WIDTH + GRID_SPACING) + GRID_SPACING;
                    int rel_y = row * (THUMB_HEIGHT + GRID_SPACING) + GRID_SPACING;

                    child->set_position(x() + rel_x, y() + rel_y);
                    child->set_size(THUMB_WIDTH, THUMB_HEIGHT);
                    count++;
                }
            }
        }
    };

    // --- ImagesView ---
    ImagesView::ImagesView()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
        set_border_color(Color(0.8f, 0.8f, 0.8f, 1.0f));
        set_border_width(1);

        if (application())
        {
            set_background_color(theme_manager()->get_color("textbox_bg"));
            set_border_color(theme_manager()->get_color("window_border"));
        }

        auto scroll = std::make_unique<ScrollArea>();
        m_scroll_area = scroll.get();

        auto grid = std::make_unique<ThumbnailGrid>();
        m_grid_container = grid.get();

        scroll->set_content(std::move(grid));
        add_child(std::move(scroll));
    }

    ImagesView::~ImagesView()
    {
        if (m_thumbnail_timer_id != 0 && application())
        {
            application()->stop_timer(m_thumbnail_timer_id);
            m_thumbnail_timer_id = 0;
        }
    }

    void ImagesView::set_path(const std::string &path)
    {
        LOG_INFO << "[ImagesView] set_path: " << path << " (Current: " << m_current_path << ")";
        if (m_current_path == path)
            return;
        m_current_path = path;

        if (m_thumbnail_timer_id != 0 && application())
        {
            application()->stop_timer(m_thumbnail_timer_id);
            m_thumbnail_timer_id = 0;
        }

        m_grid_container->clear_children();
        m_image_paths.clear();
        invalidate();

        if (path.empty() || !std::filesystem::exists(path))
            return;

        std::vector<std::string> extensions = {".jpg", ".jpeg", ".png", ".bmp", ".svg"};

        try
        {
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_regular_file())
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end())
                    {
                        std::string original = entry.path().string();
                        m_image_paths.push_back(original);

                        std::string thumb = lens::ThumbnailCache::get_thumbnail(original, lens::ThumbnailSize::Large);

                        if (thumb.empty() && lens::ThumbnailCache::is_supported(original))
                        {
                            lens::ThumbnailCache::request_thumbnail(original, lens::ThumbnailSize::Large);
                        }

                        auto item = std::make_unique<ThumbnailItem>(original, thumb);
                        item->set_position_type(horizon::WidgetPositionTypes::FREE);

                        item->when_mouse_press.connect(
                            [this, original](MouseButtonEventContext &ev)
                            {
                                if (ev.button == 0x110)
                                { // BTN_LEFT
                                    when_image_selected.run(original);
                                    ev.stop_propagation = true;
                                }
                            });

                        m_grid_container->add_child(std::move(item));
                    }
                }
            }
        }
        catch (...)
        {
            LOG_ERROR << "[ImagesView] Failed to read directory " << path;
        }

        update_layout();
        invalidate();
        start_thumbnail_watch();
    }

    void ImagesView::start_thumbnail_watch()
    {
        if (!application())
            return;

        if (m_thumbnail_timer_id != 0)
        {
            application()->stop_timer(m_thumbnail_timer_id);
            m_thumbnail_timer_id = 0;
        }

        // Check if any thumbnails are missing
        bool pending = false;
        for (const auto &original : m_image_paths)
        {
            if (lens::ThumbnailCache::is_supported(original) &&
                lens::ThumbnailCache::get_thumbnail(original, lens::ThumbnailSize::Large).empty())
            {
                pending = true;
                break;
            }
        }

        if (pending)
        {
            m_thumbnail_timer_id = application()->add_timer(1000,
                [this]()
                {
                    m_thumbnail_timer_id = 0;
                    check_thumbnails();
                });
        }
    }

    void ImagesView::check_thumbnails()
    {
        if (!application() || !m_grid_container)
            return;

        bool has_new = false;
        bool still_pending = false;

        for (auto const &child : m_grid_container->children())
        {
            if (auto *item = dynamic_cast<ThumbnailItem *>(child.get()))
            {
                std::string original = item->original_path();
                if (lens::ThumbnailCache::is_supported(original))
                {
                    std::string thumb = lens::ThumbnailCache::get_thumbnail(original, lens::ThumbnailSize::Large);
                    if (!thumb.empty())
                    {
                        item->set_thumbnail(thumb);
                        has_new = true;
                    }
                    else
                    {
                        still_pending = true;
                    }
                }
            }
        }

        if (has_new)
        {
            invalidate();
        }

        if (still_pending)
        {
            m_thumbnail_timer_id = application()->add_timer(1000,
                [this]()
                {
                    m_thumbnail_timer_id = 0;
                    check_thumbnails();
                });
        }
    }

    void ImagesView::set_application_recursive(WaylandWindow *app)
    {
        horizon::Widget::set_application_recursive(app);
        if (app)
        {
            set_background_color(theme_manager()->get_color("textbox_bg"));
            set_border_color(theme_manager()->get_color("window_border"));
            
            start_thumbnail_watch();
        }
    }

    void ImagesView::calculate_layout()
    {
        horizon::Widget::calculate_layout();
        update_layout();
    }

    void ImagesView::update_layout()
    {
        if (!m_grid_container || !m_scroll_area)
            return;

        int w = m_scroll_area->width();
        int h = m_scroll_area->height();

        if (w <= 0)
        {
            // Try to use our own width if scroll area is not ready
            w = width();
            if (w <= 0)
                return;
        }

        // Available width for items depends on whether the vertical scrollbar is visible.
        int available_width = w - SCROLLBAR_MARGIN;

        m_grid_container->set_width(available_width);

        int columns = std::max(1, (available_width + GRID_SPACING) / (THUMB_WIDTH + GRID_SPACING));
        int count = m_grid_container->children().size();
        int rows = (count + columns - 1) / columns;
        int total_height = rows * (THUMB_HEIGHT + GRID_SPACING) + BOTTOM_PADDING;

        m_grid_container->set_height(std::max(h, total_height));
        m_scroll_area->calculate_layout();

        LOG_INFO << "[ImagesView] Layout updated: w=" << available_width << " col=" << columns
                 << " items=" << count << " height=" << total_height;
    }
} // namespace horizon::preferences
