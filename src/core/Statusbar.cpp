#include "horizon/Widget.hpp"
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Statusbar.hpp>
#include <memory>
#include <unistd.h>

namespace horizon
{
    Statusbar::Statusbar()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    }

    void Statusbar::set_text(std::string text)
    {
        m_text = std::move(text);
        invalidate();
    }

    const std::string &Statusbar::text() const
    {
        return m_text;
    }

    void Statusbar::render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force)
    {
        Widget::render(gc, cx, cy, cw, ch, force);
    }

    void Statusbar::draw(GraphicsContext &gc)
    {
        // Dibujarmos una barra de titulo como la de mac os mountain lion.
        auto *tm = application()->theme_manager.get();

        set_background_colors(tm->get_color("titlebar_bg1"), tm->get_color("titlebar_bg2"));
        set_border_color(tm->get_color("titlebar_border"));

        // Ensure 10px radius on bottom corners
        m_corner_radius = CornerRadius(0, 0, 10, 10);

        // Fill background with gradient and rounded corners
        gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_available_draw_width,
                                  m_available_draw_height, m_bg2, m_bg1, true, m_corner_radius);

        // Draw border around the statusbar to make it visible
        gc.setColor(m_border_color);
        gc.drawRect(m_x, m_y, m_width, m_height, m_corner_radius, 1.0f);

        // Draw text
        if (!m_text.empty())
        {
            gc.setColor(Color(0.2f, 0.2f, 0.2f)); // Dark grey for status text
            gc.drawText(m_start_draw_x + 10, m_start_draw_y + 18, m_text.c_str());
        }
    }

} // namespace horizon