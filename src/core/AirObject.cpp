#include <horizon/AirObject.hpp>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    AirObject::AirObject() : Widget()
    {
        m_corner_radius = CornerRadius(6, 6, 6, 6);

        // Basic event handling for visual feedback
        when_mouse_enter.connect(
            [this](EventContext &)
            {
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_ENTER));
                invalidate();
            });
        when_mouse_leave.connect(
            [this](EventContext &)
            {
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_LEAVE));
                invalidate();
            });
        when_mouse_press.connect(
            [this](MouseButtonEventContext &)
            {
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_PRESS));
                invalidate();
            });
        when_mouse_release.connect(
            [this](MouseButtonEventContext &)
            {
                set_draw_state(get_draw_state(WidgetEvent::MOUSE_RELEASE));
                invalidate();
            });
    }

    void AirObject::set_corner_radius(CornerRadius radius)
    {
        if (m_corner_radius.top_left == radius.top_left &&
            m_corner_radius.top_right == radius.top_right &&
            m_corner_radius.bottom_left == radius.bottom_left &&
            m_corner_radius.bottom_right == radius.bottom_right)
            return;

        m_corner_radius = radius;
        invalidate();
    }

    CornerRadius AirObject::corner_radius() const
    {
        return m_corner_radius;
    }

    void AirObject::draw(GraphicsContext &gc)
    {
        auto *tm = theme_manager();

        Color c1 = tm->get_color("air_default1");
        Color c2 = tm->get_color("air_default2");
        Color window_bg = tm->get_color("window_bg");
        Color border_color = tm->get_color("window_border");
        Color air_brd = tm->get_color("air_border");

        switch (m_accent_color)
        {
        case WidgetAccentColor::Default:
            c1 = tm->get_color("air_default1");
            c2 = tm->get_color("air_default2");
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

        // For the "Air" look, we use very light versions of the accent colors
        Color top1 = c1;
        Color top2 = c2;
        Color bot1 = c1.darker(5.0f); // Very close to top2 for a subtle transition
        Color bot2 = c2.darker(5.0f);

        /*if (m_accent_color == WidgetAccentColor::Default)
        {
            top1 = Color(1.0f, 1.0f, 1.0f, 1.0f);
            top2 = Color(0.96f, 0.96f, 0.96f, 1.0f);
            bot1 = Color(0.94f, 0.94f, 0.94f, 1.0f);
            bot2 = Color(0.90f, 0.90f, 0.90f, 1.0f);
        }*/

        // Adjust colors based on state
        if (m_draw_state == WidgetDrawState::HOVERED)
        {
            top1 = top1.lighter(5.0f);
            top2 = top2.lighter(5.0f);
            bot1 = bot1.lighter(5.0f);
            bot2 = bot2.lighter(5.0f);
        }
        else if (m_draw_state == WidgetDrawState::PRESSED)
        {
            top1 = bot1;
            top2 = bot2;
            bot1 = bot2.darker(5.0f);
            bot2 = bot2.darker(10.0f);
        }

        int halfHeight = m_height / 2;

        // 1. Top half fill
        CornerRadius top_radius(m_corner_radius.top_left, m_corner_radius.top_right, 0, 0);
        gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_width, halfHeight, top1, top2,
                                  true, top_radius);

        // 2. Bottom half fill
        CornerRadius bot_radius(0, 0, m_corner_radius.bottom_right, m_corner_radius.bottom_left);
        gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y + halfHeight, m_width,
                                  m_height - halfHeight, bot1, bot2, true, bot_radius);

        // 3. Subtle white inner highlight (more pronounced at the top)
        /*gc.setColor(air_brd.lighter(20.0f));
        gc.drawRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2, m_height - 2,
                    m_corner_radius, 1.0f);*/

        // 4. Outer border
        Color final_border = air_brd;
        gc.setColor(final_border);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height, m_corner_radius, 1.25f);
    }
} // namespace horizon
