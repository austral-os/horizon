#include <horizon/LoadingBar.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Application.hpp>
#include <cmath>

namespace horizon
{
    LoadingBar::LoadingBar() : Widget()
    {
        // Default height for a loading bar
        m_height = 20;
    }

    LoadingBar::~LoadingBar()
    {
        if (m_app && m_timer_id != 0)
        {
            m_app->stop_timer(m_timer_id);
            m_timer_id = 0;
        }
    }

    void LoadingBar::set_application_recursive(WaylandWindow *app)
    {
        if (m_app && m_timer_id != 0)
        {
            m_app->stop_timer(m_timer_id);
            m_timer_id = 0;
        }

        Widget::set_application_recursive(app);

        if (m_app)
        {
            m_timer_id = m_app->add_timer(
                30, // Higher frequency for smoother animation (30ms = ~33 fps)
                [this]()
                {
                    if (!is_effectively_visible())
                        return;

                    // Animation speed: move 1 pixel per frame
                    m_animation_offset += 1.5f;
                    if (m_animation_offset >= 28.0f) // Matches stripe_spacing
                        m_animation_offset = 0.0f;

                    invalidate();
                },
                true); // repeat = true
        }
    }

    void LoadingBar::draw(GraphicsContext &gc)
    {
        auto *tm = theme_manager();

        // Classic Mac OS X "Tiger" blue colors (revised for even lighter light parts)
        Color c1 = Color(0.65f, 0.85f, 1.0f); // Very light blue (almost white)
        Color c2 = Color(0.12f, 0.48f, 0.88f); // Stronger blue for contrast

        // If a specific accent is set, use theme colors, otherwise use the Mac Blue
        if (m_accent_color != WidgetAccentColor::Default)
        {
            switch (m_accent_color)
            {
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
            default:
                break;
            }
        }

        CornerRadius radius = 2; // Pill shape but not too round

        // --- Recessed Track (Background) ---
        Color track_top = tm->get_color("default1").darker(0.1f);
        Color track_bottom = tm->get_color("default2").lighter(0.1f);
        gc.fillLinearGradientRect(m_x, m_y, m_width, m_height, track_top, track_bottom, true, radius);

        // Inner shadow/depth effect for track
        gc.setColor(Color(0, 0, 0, 0.4f));
        gc.drawRect(m_x, m_y, m_width, m_height, radius, 1.0f);
        gc.setColor(Color(1, 1, 1, 0.3f));
        gc.drawLine(m_x + radius.top_left, m_y + m_height, m_x + m_width - radius.top_right, m_y + m_height, 1.0f);

        // --- Animated Busy Bar ---
        gc.save();
        gc.clipRoundedRect(m_x, m_y, m_width, m_height, radius);

        // 1. Base Gradient
        gc.fillLinearGradientRect(m_x, m_y, m_width, m_height, c1, c2, true, 0);

        // 2. Diagonal Stripes (Barber Pole style)
        gc.setColor(Color(1, 1, 1, 0.4f)); // Brighter white stripes
        int stripe_spacing = 30;
        float stripe_width = 14.0f;

        for (float dx = -m_height * 2 + m_animation_offset; dx < m_width + m_height;
             dx += (float)stripe_spacing)
        {
            gc.drawLine(m_x + (int)dx, m_y + m_height + 10, m_x + (int)dx + m_height + 10, m_y - 10,
                        stripe_width);
        }

        // 3. Double Gloss Effect (High gloss for Tiger style)
        // Top highlight (glass glare)
        gc.fillLinearGradientRect(m_x, m_y, m_width, m_height / 2, Color(1, 1, 1, 0.5f), Color(1, 1, 1, 0.05f), true, 0);

        // Bottom glow
        gc.fillLinearGradientRect(m_x, m_y + m_height * 0.7f, m_width, m_height * 0.3f, Color(1, 1, 1, 0.0f), Color(1, 1, 1, 0.25f), true, 0);

        gc.restore();

        // Crisp border for the entire widget
        gc.setColor(c2.darker(0.3f).with_alpha(0.7f));
        gc.drawRect(m_x, m_y, m_width, m_height, radius, 1.0f);
    }
} // namespace horizon
