#include <ImagesView.hpp>
#include <algorithm>
#include <filesystem>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Color.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/arkutils/Thumbnailer.hpp>

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
        img->set_path(thumbnail_path);
        img->set_mode(ImageMode::Fit); // Maintain aspect ratio
        img->set_position_type(horizon::WidgetPositionTypes::FILL);
        m_image = img.get();
        add_child(std::move(img));

        set_cursor_type(CursorType::Pointer);
    }

    void ThumbnailItem::draw(horizon::GraphicsContext &gc)
    {
        horizon::Widget::draw(gc);
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
            set_background_color(application()->theme_manager->get_color("textbox_bg"));
            set_border_color(application()->theme_manager->get_color("window_border"));
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
        LOG_INFO << "[ImagesView] Destructor called, stopping loading thread...";
        m_alive->store(false);
        stop_loading();
    }

    void ImagesView::set_path(const std::string &path)
    {
        LOG_INFO << "[ImagesView] set_path: " << path << " (Current: " << m_current_path << ")";
        if (m_current_path == path)
            return;
        m_current_path = path;

        stop_loading();

        {
            std::lock_guard<std::mutex> lock(m_pending_mutex);
            m_pending_thumbnails.clear();
        }

        m_grid_container->clear_children();
        invalidate();

        if (!path.empty())
        {
            LOG_INFO << "[ImagesView] Dispatching start_loading for " << path;
            start_loading(path);
        }
    }

    void ImagesView::start_loading(const std::string &path)
    {
        m_stop_requested = false;
        m_loading_thread = std::thread(
            [this, path]()
            {
                try
                {
                    std::vector<std::string> extensions = {".jpg", ".jpeg", ".png", ".bmp", ".svg"};

                    if (!std::filesystem::exists(path))
                        return;

                    for (const auto &entry : std::filesystem::directory_iterator(path))
                    {
                        if (m_stop_requested)
                            break;

                        if (entry.is_regular_file())
                        {
                            std::string ext = entry.path().extension().string();
                            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                            if (std::find(extensions.begin(), extensions.end(), ext) !=
                                extensions.end())
                            {
                                std::string original = entry.path().string();
                                LOG_INFO << "[ImagesView] Found image: " << original;
                                std::string thumb = arkutils::Thumbnailer::generate(
                                    original, THUMB_WIDTH, THUMB_HEIGHT);
                                LOG_INFO << "[ImagesView] Thumbnail generated: " << thumb;

                                if (!thumb.empty())
                                {
                                    add_thumbnail_safe(original, thumb);
                                }
                            }
                        }
                    }
                }
                catch (...)
                {
                }
            });
    }

    void ImagesView::stop_loading()
    {
        m_stop_requested = true;
        if (m_loading_thread.joinable())
        {
            m_loading_thread.join();
        }
    }

    void ImagesView::add_thumbnail_safe(const std::string &original_path,
                                        const std::string &thumbnail_path)
    {
        if (!m_alive->load())
            return;

        auto *app = WaylandWindow::get_active_window();
        if (app)
        {
            LOG_INFO << "[ImagesView] add_thumbnail_safe: Posting task for " << original_path;
            app->post_task(
                [this, original_path, thumbnail_path, alive = m_alive]()
                {
                    if (!alive->load() || m_stop_requested)
                        return;
                    LOG_INFO << "[ImagesView] Task executing for " << original_path;

                    auto item = std::make_unique<ThumbnailItem>(original_path, thumbnail_path);
                    item->set_position_type(horizon::WidgetPositionTypes::FREE);

                    std::string path = original_path;
                    item->when_mouse_press.connect(
                        [this, path](MouseButtonEventContext &ev)
                        {
                            if (ev.button == 0x110)
                            { // BTN_LEFT
                                when_image_selected.run(path);
                                ev.stop_propagation = true;
                            }
                        });

                    m_grid_container->add_child(std::move(item));
                    update_layout();
                    invalidate();
                });
        }
        else
        {
            LOG_INFO << "[ImagesView] add_thumbnail_safe: Buffering " << original_path
                     << " (No Application context)";
            // Store pending thumbnail for when we are attached to window
            std::lock_guard<std::mutex> lock(m_pending_mutex);
            m_pending_thumbnails.push_back({original_path, thumbnail_path});
        }
    }

    void ImagesView::set_application_recursive(WaylandWindow *app)
    {
        horizon::Widget::set_application_recursive(app);
        if (app)
        {
            set_background_color(app->theme_manager->get_color("textbox_bg"));
            set_border_color(app->theme_manager->get_color("window_border"));

            std::lock_guard<std::mutex> lock(m_pending_mutex);
            for (const auto &pending : m_pending_thumbnails)
            {
                add_thumbnail_safe(pending.original, pending.thumb);
            }
            m_pending_thumbnails.clear();
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
