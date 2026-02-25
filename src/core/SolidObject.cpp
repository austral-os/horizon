#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Widget.hpp>

#ifdef DEBUG
#include <iostream>
#endif

namespace horizon
{
    SolidObject::SolidObject() : Widget()
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

    void SolidObject::draw(GraphicsContext &gc)
    {

        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");

        Color window_bg = tm->get_color("window_bg");
        Color c1 = tm->get_color("window_bg").lighter(10.f);

        Color window_fg = tm->get_color("window_fg");
        Color shadow_color = tm->get_color("window_border");
        Color border_color = shadow_color.darker(40.0f);

        Color top1 = c1;

        Color highlight = window_bg;
        Color highlight2 = highlight.with_alpha(0.3f);
        Color text_color = window_fg;

#ifdef DEBUG

        printf("top1: %s\n", top1.to_hex().c_str());
        printf("highlight: %s\n", highlight.to_hex().c_str());
        printf("highlight2: %s\n", highlight2.to_hex().c_str());
        printf("text_color: %s\n", text_color.to_hex().c_str());

#endif
        int radius = 6;

        // clear background
        gc.setColor(window_bg);
        gc.fillRect(m_x, m_y, m_width, m_height);

        // Outer border (gray/black)
        gc.setColor(border_color);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3,
                    {radius, radius, radius, radius}, 1.0f);

        if (is_hovered())
        {
            highlight = highlight.lighter(100.0f);
            highlight2 = highlight.with_alpha(0.5f);
        }

        // Top half: gradient from white to very light gray (creates a glass reflection effect)
        int halfHeight = m_height / 2;
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2, halfHeight,
                                  top1.darker(2.f), top1.darker(10.f), true,
                                  {radius - 1, radius - 1, 0, 0});

        // Bottom half: gradient starting slightly darker and lightening towards the bottom
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + halfHeight, m_width - 2,
                                  m_height - 4 - halfHeight, top1.darker(10.f), top1.darker(10.f),
                                  true, {0, 0, radius - 1, radius - 1});

        // Borde inferior
        gc.setColor(border_color.lighter(80.f));
        gc.fillRect(m_x + radius, m_start_draw_y + m_available_draw_height - 2,
                    m_width - radius * 2, 2);
    }

} // namespace horizon