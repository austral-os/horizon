#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    SolidObject::SolidObject() : Widget()
    {
        m_corner_radius = CornerRadius();

        when_mouse_enter.connect(
            [this](EventContext &)
            {
                // Redibuja el widget
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_ENTER));
                invalidate();
            });
        when_mouse_leave.connect(
            [this](EventContext &)
            {
                // Redibuja el widget
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_LEAVE));
                invalidate();
            });

        when_mouse_press.connect(
            [this](EventContext &)
            {
                // Redibuja el widget
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_PRESS));
                invalidate();
            });

        when_mouse_release.connect(
            [this](EventContext &)
            {
                // Redibuja el widget
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_RELEASE));
                invalidate();
            });
    }

    void SolidObject::set_corner_radius(CornerRadius radius)
    {
        m_corner_radius = radius;
    }

    CornerRadius SolidObject::corner_radius() const
    {
        return m_corner_radius;
    }

    void SolidObject::draw(GraphicsContext &gc)
    {

        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");

        Color window_bg = tm->get_color("window_bg");
        Color c1 = tm->get_color("default1").lighter(85.f);

        Color window_fg = tm->get_color("window_fg");
        Color shadow_color = tm->get_color("window_border");
        Color border_color = shadow_color.darker(40.0f);

        switch (m_accent_color)
        {
        case WidgetAccentColor::Default:
            c1 = tm->get_color("default1").lighter(85.f);
            break;
        case WidgetAccentColor::Primary:
            c1 = tm->get_color("primary1").lighter(80.f);
            break;
        case WidgetAccentColor::Secondary:
            c1 = tm->get_color("secondary1").lighter(80.f);
            break;
        case WidgetAccentColor::Success:
            c1 = tm->get_color("success1").lighter(80.f);
            break;
        case WidgetAccentColor::Warning:
            c1 = tm->get_color("warning1").lighter(80.f);
            break;
        case WidgetAccentColor::Error:
            c1 = tm->get_color("error1").lighter(80.f);
            break;
        case WidgetAccentColor::Info:
            c1 = tm->get_color("info1").lighter(80.f);
            break;
        }

        if (m_draw_state == WidgetDrawState::NORMAL)
        {

            Color top1 = c1;

            Color highlight = window_bg;
            Color highlight2 = highlight.with_alpha(0.3f);
            Color text_color = window_fg;

            // clear background
            gc.setColor(window_bg);
            gc.fillRect(m_x, m_y, m_width, m_height, m_corner_radius);

            // Outer border (gray/black)
            gc.setColor(border_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3, m_corner_radius,
                        1.0f);

            // Top half: gradient from white to very light gray (creates a glass reflection effect)
            int halfHeight = m_height / 2;
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2,
                                      halfHeight, highlight.darker(2.f), highlight.darker(10.f),
                                      true,
                                      {m_corner_radius.top_left, m_corner_radius.top_right, 0, 0});

            // Bottom half: gradient starting slightly darker and lightening towards the bottom
            gc.fillLinearGradientRect(
                m_start_draw_x + 1, m_start_draw_y + halfHeight, m_width - 2,
                m_height - 4 - halfHeight, top1.darker(10.f), top1.darker(10.f), true,
                {0, 0, m_corner_radius.bottom_right, m_corner_radius.bottom_left});

            // Borde inferior
            gc.setColor(border_color.lighter(80.f));
            gc.fillRect(m_x + m_corner_radius.bottom_left,
                        m_start_draw_y + m_available_draw_height - 2,
                        m_width - m_corner_radius.bottom_left - m_corner_radius.bottom_right, 2);
        }
        else if (m_draw_state == WidgetDrawState::HOVERED)
        {
            Color top1 = c1;

            Color highlight = window_bg;
            Color highlight2 = highlight.with_alpha(0.3f);
            Color text_color = window_fg;

            // clear background
            gc.setColor(window_bg);
            gc.fillRect(m_x, m_y, m_width, m_height, m_corner_radius);

            // Outer border (gray/black)
            gc.setColor(border_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3, m_corner_radius,
                        1.0f);

            highlight = highlight.lighter(100.0f);
            highlight2 = highlight.with_alpha(0.5f);

            // Top half: gradient from white to very light gray (creates a glass reflection effect)
            int halfHeight = m_height / 2;
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2,
                                      halfHeight, highlight.darker(2.f), highlight.darker(10.f),
                                      true,
                                      {m_corner_radius.top_left, m_corner_radius.top_right, 0, 0});

            // Bottom half: gradient starting slightly darker and lightening towards the bottom
            gc.fillLinearGradientRect(
                m_start_draw_x + 1, m_start_draw_y + halfHeight, m_width - 2,
                m_height - 4 - halfHeight, top1.darker(10.f), top1.darker(10.f), true,
                {0, 0, m_corner_radius.bottom_right, m_corner_radius.bottom_left});

            // Borde inferior
            gc.setColor(border_color.lighter(80.f));
            gc.fillRect(m_x + m_corner_radius.bottom_left,
                        m_start_draw_y + m_available_draw_height - 2,
                        m_width - m_corner_radius.bottom_left - m_corner_radius.bottom_right, 2);
        }
        else if (m_draw_state == WidgetDrawState::PRESSED)
        {
            Color top1 = c1;

            Color highlight = window_bg;
            Color highlight2 = highlight.with_alpha(0.3f);
            Color text_color = window_fg;

            // clear background
            gc.setColor(window_bg);
            gc.fillRect(m_x, m_y, m_width, m_height, m_corner_radius);

            // Outer border (gray/black)
            gc.setColor(border_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3, m_corner_radius,
                        1.0f);

            highlight = highlight.darker(60.0f);
            highlight2 = highlight.with_alpha(0.5f);

            // Top half: gradient from white to very light gray (creates a glass reflection effect)
            int halfHeight = m_height / 2;
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2,
                                      m_height - 4, highlight, highlight.with_alpha(0.1f), true,
                                      {m_corner_radius.top_left, m_corner_radius.top_right,
                                       m_corner_radius.bottom_right, m_corner_radius.bottom_left});

            // Borde inferior
            gc.drawLinearGradientRect(m_start_draw_x + 2, m_start_draw_y + 2, m_width - 2,
                                      m_height - 3, border_color, border_color.with_alpha(0.f),
                                      3.0f, true,
                                      {m_corner_radius.top_left, m_corner_radius.top_right,
                                       m_corner_radius.bottom_right, m_corner_radius.bottom_left});
        }
    }

} // namespace horizon