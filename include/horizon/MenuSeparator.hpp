#pragma once
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{

    class MenuSeparator : public Widget
    {
    public:
        MenuSeparator() : Widget()
        {
            set_size(100, 10); // Default height
        }

        void draw(GraphicsContext &gc) override
        {
            int mid_y = m_start_draw_y + m_height / 2;
            gc.setColor(Color(0.85f, 0.85f, 0.85f, 1.0f)); // Light gray line
            gc.drawLine(m_start_draw_x + 5, mid_y, m_start_draw_x + m_width - 5, mid_y);
        }
    };

} // namespace horizon
