#pragma once

#include <horizon/Color.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    /**
     * @class TableRow
     * @brief A simple horizontal container for table cells with background support.
     */
    class TableRow : public Widget
    {
    public:
        TableRow() : Widget()
        {
            m_layout_type = WIDGET_LAYOUT_HORIZONTAL;
            m_position_type = FILL;
            m_spacing = 0;
            m_margin = 0;
        }

        void set_background_color(Color c)
        {
            m_bg_color = c;
            m_has_bg = true;
            invalidate();
        }

    protected:
        void draw(GraphicsContext &gc) override
        {
            if (m_has_bg)
            {
                gc.setColor(m_bg_color);
                gc.fillRect(m_x, m_y, m_width, m_height);
            }
            Widget::draw(gc);
        }

    private:
        Color m_bg_color;
        bool m_has_bg{false};
    };
} // namespace horizon
