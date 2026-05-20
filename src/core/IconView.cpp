#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/IconView.hpp>

namespace horizon
{
    IconViewBase::IconViewBase() : Widget()
    {
        // Theme color will be applied when added to an application


        auto scroll_area = std::make_unique<ScrollArea>();
        scroll_area->set_position_type(FREE);
        m_scroll_area = scroll_area.get();

        auto content_pane = std::make_unique<Widget>();
        content_pane->set_position_type(FREE);
        m_content_pane = content_pane.get();

        m_scroll_area->set_content(std::move(content_pane));
        add_child(std::move(scroll_area));

        m_scroll_area->when_mouse_press.connect([this](MouseButtonEventContext &ctx) {
            if (ctx.button == 0x110 || ctx.button == 0x111)
            {
                set_selected_index(-1);
            }
        });
    }

    void IconViewBase::set_zoom(float zoom)
    {
        float clamped_zoom = std::max(0.2f, std::min(zoom, 5.0f));
        if (m_zoom != clamped_zoom)
        {
            m_zoom = clamped_zoom;
            rebuild_items();
        }
    }

    float IconViewBase::zoom() const
    {
        return m_zoom;
    }

    void IconViewBase::set_transparent(bool transparent)
    {
        m_transparent = transparent;
        if (transparent)
        {
            set_background_color(Color(0.0f, 0.0f, 0.0f, 0.0f));
        }
        else if (theme_manager())
        {
            set_background_color(theme_manager()->get_color("textbox_bg"));
        }
        else
        {
            set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f)); // fallback
        }
    }

    void IconViewBase::set_application_recursive(WaylandWindow *app)
    {
        Widget::set_application_recursive(app);
        if (theme_manager() && !m_transparent)
        {
            set_background_color(theme_manager()->get_color("textbox_bg"));
        }
    }

    void IconViewBase::set_selected_index(int index)
    {
        if (m_selected_index != index)
        {
            m_selected_index = index;

            // Defer rebuild to avoid destroying the widget tree during event handling
            if (application())
            {
                application()->add_timer(0, [this]() { rebuild_items(); });
            }
            else
            {
                rebuild_items();
            }
        }
    }

    int IconViewBase::selected_index() const
    {
        return m_selected_index;
    }
    void IconViewBase::set_side_margin(int margin)
    {
        if (m_side_margin != margin)
        {
            m_side_margin = margin;
            invalidate();
            calculate_layout();
        }
    }

    int IconViewBase::side_margin() const
    {
        return m_side_margin;
    }

    void IconViewBase::set_item_size(int width, int height)
    {
        BASE_ITEM_WIDTH = width;
        BASE_ITEM_HEIGHT = height;
        invalidate();
        calculate_layout();
    }

    int IconViewBase::get_theme_font_size(const std::string &role) const
    {
        if (!theme_manager())
            return 12;

        auto fd = theme_manager()->get_font(role);
        if (fd.size > 0)
            return fd.size;

        // Fallback to window role
        fd = theme_manager()->get_font("window");
        if (fd.size > 0)
            return fd.size;

        return 12; // Hard fallback
    }

    void IconViewBase::calculate_layout()
    {
        Widget::calculate_layout();

        if (m_width <= 0 || m_height <= 0)
        {
            if (m_parent && m_parent->width() > 0 && m_parent->height() > 0)
            {
                m_width = m_parent->width();
                m_height = m_parent->height();
            }
            else
            {
                return;
            }
        }

        if (m_scroll_area)
        {
            m_scroll_area->set_position(m_x, m_y);
            m_scroll_area->set_size(m_width, m_height);
        }

        m_item_width = std::max(16, static_cast<int>(BASE_ITEM_WIDTH * m_zoom));
        m_item_height = std::max(16, static_cast<int>(BASE_ITEM_HEIGHT * m_zoom));
        m_grid_spacing = std::max(0, static_cast<int>(BASE_GRID_SPACING * m_zoom));

        int side_margin = std::max(0, static_cast<int>(m_side_margin * m_zoom));
        int available_width = m_width - 2 * side_margin;

        if (available_width <= 0)
            available_width = m_width;

        int columns = std::max(1, available_width / (m_item_width + m_grid_spacing));
        if (columns > 1 &&
            (columns * (m_item_width + m_grid_spacing) - m_grid_spacing) > available_width)
        {
            columns--;
            if (columns < 1)
                columns = 1;
        }

        // Center the grid horizontally
        int grid_width = columns * (m_item_width + m_grid_spacing) - m_grid_spacing;
        int start_x = side_margin + (available_width - grid_width) / 2;

        int scroll_x = m_scroll_area ? m_scroll_area->scroll_x() : 0;
        int scroll_y = m_scroll_area ? m_scroll_area->scroll_y() : 0;

        int actual_spacing = m_grid_spacing;
        if (columns > 1)
        {
            actual_spacing = (available_width - columns * m_item_width) / (columns - 1);
            start_x = side_margin;
        }

        int current_y = side_margin;
        auto &children = m_content_pane->children();

        for (int i = 0; i < (int)children.size(); i += columns)
        {
            int row_max_height = 0;
            int row_end = std::min(i + columns, (int)children.size());

            for (int j = i; j < row_end; ++j)
            {
                row_max_height =
                    std::max(row_max_height, children[j]->preferred_height(m_item_width));
            }

            for (int j = i; j < row_end; ++j)
            {
                int col = j - i;
                int x = m_x + start_x + col * (m_item_width + actual_spacing) - scroll_x;
                int y = m_y + current_y - scroll_y;
                children[j]->set_position(x, y);
                children[j]->set_size(m_item_width, row_max_height);
            }

            current_y += row_max_height + m_grid_spacing;
        }

        int needed_height = current_y + side_margin - (children.empty() ? 0 : m_grid_spacing);
        needed_height = std::min(1000000, std::max(m_height, needed_height));
        m_content_pane->set_size(m_width, needed_height);
    }

} // namespace horizon
