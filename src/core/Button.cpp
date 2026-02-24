#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>
#include <iostream>

namespace horizon
{
    Button::Button() : Widget()
    {
        when_mouse_enter.connect(
            [this](EventContext &)
            {
                // Redibuja el widget
                invalidate();
            });
        when_mouse_leave.connect(
            [this](EventContext &)
            {
                // Redibuja el widget
                invalidate();
            });
    }

    void Button::draw(GraphicsContext &gc)
    {

        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");

        Color window_bg = tm->get_color("primary");
        Color window_fg = tm->get_color("window_fg");
        Color shadow_color = tm->get_color("window_border");
        Color border_color = shadow_color.darker(50.0f);

        Color top1 = window_bg;
        Color top2 = top1.lighter(50.f);
        Color bot1 = top2;
        Color bot2 = window_bg.lighter(10.0f);
        Color highlight = window_bg;
        Color highlight2 = highlight.with_alpha(50.0f);
        Color text_color = window_fg;

        int radius = m_height / 2;

        // Borde exterior adicional (sombra inferior, color más claro que el negro)
        gc.setColor(shadow_color);
        gc.drawRect(m_start_draw_x, m_start_draw_y - 1, m_width, m_height,
                    {radius, radius, radius, radius}, 1.0f);

        // Outer border (gray/black)
        gc.setColor(border_color);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3,
                    {radius, radius, radius, radius}, 1.5f);

        if (is_hovered())
        {
            highlight = highlight.lighter(100.0f);
            highlight2 = highlight.with_alpha(50.0f);
        }

        // Top half: gradient from white to very light gray (creates a glass reflection effect)
        int halfHeight = m_height / 2;
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2, halfHeight,
                                  top1, top2, true, {radius - 1, radius - 1, 0, 0});

        // Bottom half: gradient starting slightly darker and lightening towards the bottom
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + halfHeight, m_width - 2,
                                  m_height - 4 - halfHeight, bot1, bot2, true,
                                  {0, 0, radius - 1, radius - 1});

        // Inner top highlight (simulates strong light reflection on top of the glass)
        int h_margin_x = radius / 4; // Margen a los lados para que el brillo sea más chico
        int h_width = m_width - (h_margin_x * 2);
        int h_height = halfHeight - 2;       // Un poco más chico que la altura de la mitad superior
        int h_radius_top = radius - 2;       // El radio superior se adapta al botón
        int h_radius_bot = h_radius_top / 2; // El radio inferior es mucho más curvo/chico

        /* gc.fillLinearGradientRect(m_start_draw_x + h_margin_x, m_start_draw_y + 2, h_width,
                                       h_height, highlight, // Blanco sólido arriba
                                       highlight2,          // Blanco casi transparente abajo
                                       true,
                                       {h_radius_top, h_radius_top, h_radius_bot, h_radius_bot});
        */
        // Center the text
        TextMetrics metrics = gc.getTextMetrics(m_text.c_str(), font.family.c_str(), font.size,
                                                FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        int text_x = m_start_draw_x + (m_width / 2) - (metrics.width / 2);
        int text_y = m_start_draw_y + (m_height / 2) + (metrics.height / 2) - 3;

        // Draw the text
        gc.setDrawFont(font.family.c_str(), font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
        gc.setColor(text_color);
        gc.drawText(text_x, text_y, m_text.c_str());
    }

    void Button::set_text(std::string text)
    {
        m_text = std::move(text);
    }

    const std::string &Button::text() const
    {
        return m_text;
    }

} // namespace horizon