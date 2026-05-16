#pragma once
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ThemeManager.hpp>
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
            Color line_color(0.85f, 0.85f, 0.85f, 1.0f); // Light gray line

            if (application() && application()->theme_manager)
            {
                line_color = application()->theme_manager->get_color("menu_separator");
            }

            gc.setColor(line_color);
            gc.drawLine(m_start_draw_x + 5, mid_y, m_start_draw_x + m_width - 5, mid_y);
        }
    };

} // namespace horizon
