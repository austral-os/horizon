#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    AquaObject::AquaObject() : Widget()
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
            [this](MouseButtonEventContext &)
            {
                // Redibuja el widget
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_PRESS));
                invalidate();
            });

        when_mouse_release.connect(
            [this](MouseButtonEventContext &)
            {
                // Redibuja el widget
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_RELEASE));
                invalidate();
            });
    }

    void AquaObject::set_corner_radius(CornerRadius radius)
    {
        m_corner_radius = radius;
    }

    CornerRadius AquaObject::corner_radius() const
    {
        return m_corner_radius;
    }

    void AquaObject::draw(GraphicsContext &gc)
    {

        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");

        Color window_bg = tm->get_color("window_bg");
        Color c1 = tm->get_color("default1");
        Color c2 = tm->get_color("default2");
        Color window_fg = tm->get_color("window_fg");
        Color shadow_color = tm->get_color("window_border");
        Color border_color = shadow_color.darker(10.0f);

        switch (m_accent_color)
        {
        case WidgetAccentColor::Default:
            c1 = tm->get_color("default1");
            c2 = tm->get_color("default2");
            break;
        case WidgetAccentColor::Primary:
            c1 = tm->get_color("primary1");
            c2 = tm->get_color("primary2");
            break;
        case WidgetAccentColor::Secondary:
            c1 = tm->get_color("secondary1");
            c2 = tm->get_color("secondary2");
            break;
        case WidgetAccentColor::Success:
            c1 = tm->get_color("success1");
            c2 = tm->get_color("success2");
            break;
        case WidgetAccentColor::Warning:
            c1 = tm->get_color("warning1");
            c2 = tm->get_color("warning2");
            break;
        case WidgetAccentColor::Error:
            c1 = tm->get_color("error1");
            c2 = tm->get_color("error2");
            break;
        case WidgetAccentColor::Info:
            c1 = tm->get_color("info1");
            c2 = tm->get_color("info2");
            break;
        }

        if (m_draw_state == WidgetDrawState::NORMAL)
        {

            Color top1 = c1;
            Color top2 = c2;
            Color bot1 = c2;
            Color bot2 = c2.lighter(30.0f);
            Color highlight = window_bg;
            Color highlight2 = highlight.with_alpha(0.3f);
            Color text_color = window_fg;

            // clear background
            gc.setColor(window_bg);
            gc.fillRect(m_x, m_y, m_width, m_height, m_corner_radius);

            // Borde exterior adicional (sombra inferior, color más claro que el negro)
            gc.setColor(shadow_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y - 1, m_width, m_height, m_corner_radius,
                        1.0f);

            // Outer border (gray/black)
            gc.setColor(border_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3, m_corner_radius,
                        1.5f);

            // Top half: gradient from white to very light gray (creates a glass reflection effect)
            int halfHeight = m_height / 2;
            CornerRadius top_radius(std::max(0, m_corner_radius.top_left - 1),
                                    std::max(0, m_corner_radius.top_right - 1), 0, 0);
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2,
                                      halfHeight, top1, top2, true, top_radius);

            // Bottom half: gradient starting slightly darker and lightening towards the bottom
            CornerRadius bot_radius(0, 0, std::max(0, m_corner_radius.bottom_right - 1),
                                    std::max(0, m_corner_radius.bottom_left - 1));
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + halfHeight, m_width - 2,
                                      m_height - 4 - halfHeight, bot1, bot2, true, bot_radius);

            // Inner top highlight (simulates strong light reflection on top of the glass)
            int h_radius_val = std::min(m_corner_radius.top_left, m_corner_radius.top_right);
            int h_margin_x =
                h_radius_val / 4; // Margen a los lados para que el brillo sea más chico
            int h_width = m_width - (h_margin_x * 2);
            int h_height = halfHeight * 0.8; // Un poco más chico que la altura de la mitad superior
            int h_radius_top =
                std::max(0, h_radius_val - 2);   // El radio superior se adapta al botón
            int h_radius_bot = h_radius_top / 2; // El radio inferior es mucho más curvo/chico

            gc.fillLinearGradientRect(
                m_start_draw_x + h_margin_x, m_start_draw_y + 2, h_width, h_height,
                highlight,  // Blanco sólido arriba
                highlight2, // Blanco casi transparente abajo
                true, {h_radius_top, m_corner_radius.top_right, h_radius_bot, h_radius_bot});
        }
        else if (m_draw_state == WidgetDrawState::HOVERED)
        {
            Color top1 = c1;
            Color top2 = c2;
            Color bot1 = c2;
            Color bot2 = c2.lighter(30.0f);
            Color highlight = window_bg;
            Color highlight2 = highlight.with_alpha(0.3f);
            Color text_color = window_fg;

            // clear background
            gc.setColor(window_bg);
            gc.fillRect(m_x, m_y, m_width, m_height, m_corner_radius);

            // Borde exterior adicional (sombra inferior, color más claro que el negro)
            gc.setColor(shadow_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y - 1, m_width, m_height, m_corner_radius,
                        1.0f);

            // Outer border (gray/black)
            gc.setColor(border_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3, m_corner_radius,
                        1.5f);

            highlight = highlight.lighter(100.0f);
            highlight2 = highlight.with_alpha(0.5f);

            // Top half: gradient from white to very light gray (creates a glass reflection effect)
            int halfHeight = m_height / 2;
            CornerRadius top_radius(std::max(0, m_corner_radius.top_left - 1),
                                    std::max(0, m_corner_radius.top_right - 1), 0, 0);
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2,
                                      halfHeight, top1, top2, true, top_radius);

            // Bottom half: gradient starting slightly darker and lightening towards the bottom
            CornerRadius bot_radius(0, 0, std::max(0, m_corner_radius.bottom_right - 1),
                                    std::max(0, m_corner_radius.bottom_left - 1));
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + halfHeight, m_width - 2,
                                      m_height - 4 - halfHeight, bot1, bot2, true, bot_radius);

            // Inner top highlight (simulates strong light reflection on top of the glass)
            int h_radius_val = std::min(m_corner_radius.top_left, m_corner_radius.top_right);
            int h_margin_x =
                h_radius_val / 4; // Margen a los lados para que el brillo sea más chico
            int h_width = m_width - (h_margin_x * 2);
            int h_height = halfHeight * 0.8; // Un poco más chico que la altura de la mitad superior
            int h_radius_top =
                std::max(0, h_radius_val - 2);   // El radio superior se adapta al botón
            int h_radius_bot = h_radius_top / 2; // El radio inferior es mucho más curvo/chico

            gc.fillLinearGradientRect(
                m_start_draw_x + h_margin_x, m_start_draw_y + 2, h_width, h_height,
                highlight,  // Blanco sólido arriba
                highlight2, // Blanco casi transparente abajo
                true, {h_radius_top, m_corner_radius.top_right, h_radius_bot, h_radius_bot});
        }
        else if (m_draw_state == WidgetDrawState::PRESSED)
        {
            Color top1 = c1;
            Color top2 = c2;
            Color bot1 = c2;
            Color bot2 = c2.lighter(30.0f);
            Color highlight = window_bg;
            Color highlight2 = highlight.with_alpha(0.3f);
            Color text_color = window_fg;

            // clear background
            gc.setColor(window_bg);
            gc.fillRect(m_x, m_y, m_width, m_height, m_corner_radius);

            // Borde exterior adicional (sombra inferior, color más claro que el negro)
            gc.setColor(shadow_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y - 1, m_width, m_height, m_corner_radius,
                        1.0f);

            // Outer border (gray/black)
            gc.setColor(border_color);
            gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3, m_corner_radius,
                        1.5f);

            highlight = highlight.darker(10.0f);
            highlight2 = highlight.with_alpha(0.5f);

            // Top half: gradient from white to very light gray (creates a glass reflection effect)
            int halfHeight = m_height / 2;
            CornerRadius top_radius(std::max(0, m_corner_radius.top_left - 1),
                                    std::max(0, m_corner_radius.top_right - 1), 0, 0);
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2,
                                      halfHeight - 5, top1, top2, true, top_radius);

            // Bottom half: gradient starting slightly darker and lightening towards the bottom
            CornerRadius bot_radius(0, 0, std::max(0, m_corner_radius.bottom_right - 1),
                                    std::max(0, m_corner_radius.bottom_left - 1));
            gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + halfHeight - 5,
                                      m_width - 2, m_height - halfHeight + 1, bot1, bot2, true,
                                      bot_radius);

            // Inner top highlight (simulates strong light reflection on top of the glass)
            int h_radius_val = std::min(m_corner_radius.top_left, m_corner_radius.top_right);
            int h_margin_x =
                h_radius_val / 4; // Margen a los lados para que el brillo sea más chico
            int h_width = m_width - (h_margin_x * 2);
            int h_height = halfHeight * 0.8; // Un poco más chico que la altura de la mitad superior
            int h_radius_top =
                std::max(0, h_radius_val - 2);   // El radio superior se adapta al botón
            int h_radius_bot = h_radius_top / 2; // El radio inferior es mucho más curvo/chico

            gc.fillLinearGradientRect(
                m_start_draw_x + h_margin_x, m_start_draw_y + 2, h_width, h_height,
                highlight,  // Blanco sólido arriba
                highlight2, // Blanco casi transparente abajo
                true, {h_radius_top, m_corner_radius.top_right, h_radius_bot, h_radius_bot});
        }
    }

} // namespace horizon