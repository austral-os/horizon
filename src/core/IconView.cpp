#include <algorithm>
#include <horizon/GraphicsContext.hpp>
#include <horizon/IconView.hpp>
#include <iostream>

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
        if (m_zoom != zoom)
        {
            m_zoom = zoom;
            rebuild_items();
        }
    }

    float IconViewBase::zoom() const
    {
        return m_zoom;
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

        m_item_width = static_cast<int>(BASE_ITEM_WIDTH * m_zoom);
        m_item_height = static_cast<int>(BASE_ITEM_HEIGHT * m_zoom);
        m_grid_spacing = static_cast<int>(BASE_GRID_SPACING * m_zoom);

        int available_width = m_width - 2 * m_margin;
        int columns = std::max(1, available_width / (m_item_width + m_grid_spacing));

        int current_col = 0;
        int current_row = 0;

        for (auto &child : m_content_pane->children())
        {
            int x = m_x - m_scroll_area->scroll_x() + m_margin +
                    current_col * (m_item_width + m_grid_spacing);
            int y = m_y - m_scroll_area->scroll_y() + m_margin +
                    current_row * (m_item_height + m_grid_spacing);

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
        int total_rows = (m_content_pane->children().size() + columns - 1) / columns;
        int needed_height = total_rows * (m_item_height + m_grid_spacing) + 2 * m_margin;

        m_content_pane->set_size(m_width, needed_height);
    }

    void IconViewBase::draw(GraphicsContext &gc)
    {
        gc.setColor(m_background_color);
        gc.fillRect(m_x, m_y, m_width, m_height);
    }
} // namespace horizon
