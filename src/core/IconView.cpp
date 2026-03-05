#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/IconView.hpp>

namespace horizon
{
    IconViewBase::IconViewBase() : Widget()
    {
        m_background_color = Color(1.0f, 1.0f, 1.0f, 1.0f); // Default white background

        auto scroll_area = std::make_unique<ScrollArea>();
        scroll_area->set_position_type(FILL);
        m_scroll_area = scroll_area.get();

        auto content_pane = std::make_unique<Widget>();
        content_pane->set_position_type(FREE);
        m_content_pane = content_pane.get();

        m_scroll_area->set_content(std::move(content_pane));
        add_child(std::move(scroll_area));
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
            m_scroll_area->set_size(m_width, m_height);
        }

        m_item_width = std::max(16, static_cast<int>(BASE_ITEM_WIDTH * m_zoom));
        m_item_height = std::max(16, static_cast<int>(BASE_ITEM_HEIGHT * m_zoom));
        m_grid_spacing = std::max(0, static_cast<int>(BASE_GRID_SPACING * m_zoom));

        int effective_margin = std::max(0, static_cast<int>(m_grid_spacing));
        int available_width = m_width - 2 * effective_margin;

        // Ensure available_width is sensible
        if (available_width <= 0)
            available_width = m_width;

        int columns = std::max(1, available_width / (m_item_width + m_grid_spacing));

        // Center the grid horizontally
        int grid_width = columns * (m_item_width + m_grid_spacing) - m_grid_spacing;
        int start_x = (m_width - grid_width) / 2;
        if (start_x < effective_margin)
            start_x = effective_margin;

        int current_col = 0;
        int current_row = 0;

        for (auto &child : m_content_pane->children())
        {
            int x = start_x + current_col * (m_item_width + m_grid_spacing);
            int y = effective_margin + current_row * (m_item_height + m_grid_spacing);

            child->set_position(x, y);
            child->set_size(m_item_width, m_item_height);

            current_col++;
            if (current_col >= columns)
            {
                current_col = 0;
                current_row++;
            }
        }

        // Set content height based on rows
        int num_items = (int)m_content_pane->children().size();
        int total_rows = (num_items + columns - 1) / columns;
        int needed_height = total_rows * (m_item_height + m_grid_spacing) + 2 * effective_margin;

        // Ensure we don't have crazy dimensions (max 1M pixels)
        needed_height = std::min(1000000, std::max(m_height, needed_height));

        m_content_pane->set_size(m_width, needed_height);
    }

    void IconViewBase::draw(GraphicsContext &gc)
    {
        gc.setColor(m_background_color);
        gc.fillRect(m_x, m_y, m_width, m_height);
    }
} // namespace horizon
