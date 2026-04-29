#include "horizon/WaylandWindow.hpp"
#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    ProgressBar::ProgressBar() : Widget()
    {
        // Default height for a progress bar
        m_height = 20;
    }

    ProgressBar::~ProgressBar()
    {
        if (m_app && m_timer_id != 0)
        {
            m_app->stop_timer(m_timer_id);
        }
    }

    void ProgressBar::set_application_recursive(WaylandWindow *app)
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
                60,
                [this]()
                {
                    if (!is_effectively_visible())
                        return;

                    bool needs_repaint = false;

                    // 1. Stripe animation offset
                    m_animation_offset += 2.0f;
                    if (m_animation_offset >= 25.0f) // Matches stripe_spacing
                        m_animation_offset = 0.0f;
                    needs_repaint = true;

                    // 2. Smooth progress transition
                    if (!m_is_indeterminate)
                    {
                        if (std::abs(m_progress - m_target_progress) > 0.001f)
                        {
                            float delta = m_target_progress - m_progress;
                            float step = 0.05f; // Transition speed
                            if (std::abs(delta) < step)
                                m_progress = m_target_progress;
                            else
                                m_progress += (delta > 0 ? step : -step);
                            needs_repaint = true;
                        }
                    }

                    if (needs_repaint)
                        invalidate();
                },
                true); // repeat = true
        }
    }

    void ProgressBar::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        Color bg = tm->get_color("window_bg");

        // Resolve accent colors
        Color c1, c2;
        switch (m_accent_color)
        {
        case WidgetAccentColor::Default:
            c1 = tm->get_color("primary1");
            c2 = tm->get_color("primary2");
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

        // Pill shape radius
        CornerRadius radius = 1;

        // --- Recessed Track (Background) ---
        Color track_top = tm->get_color("default1").darker(0.1f);
        Color track_bottom = tm->get_color("default2").lighter(0.1f);
        gc.fillLinearGradientRect(m_x, m_y, m_width, m_height, track_top, track_bottom, true,
                                  radius);

        // Inner shadow/depth effect (dark top, light bottom)
        gc.setColor(Color(0, 0, 0, 0.3f));
        gc.drawRect(m_x, m_y, m_width, m_height, radius, 1.0f);
        gc.setColor(Color(1, 1, 1, 0.4f));
        gc.drawLine(m_x + radius.top_left, m_y + m_height, m_x + m_width - radius.top_right,
                    m_y + m_height, 1.0f);

        bool effectively_zero = (m_progress <= 0.001f);
        bool effectively_full = (m_progress >= 0.999f);
        bool show_indeterminate = m_is_indeterminate || (effectively_zero && m_target_progress <= 0.001f);

        if (m_progress > 0.0f || show_indeterminate)
        {
            float draw_progress = show_indeterminate ? 1.0f : m_progress;
            int progress_width = (int)(m_width * std::clamp(draw_progress, 0.0f, 1.0f));
            if (progress_width < m_height && !show_indeterminate)
                progress_width = m_height;

            gc.save();
            gc.clipRoundedRect(m_x, m_y, progress_width, m_height, radius);

            // 1. Base Gradient
            gc.fillLinearGradientRect(m_x, m_y, progress_width, m_height, c1, c2, true, 0);

            // 2. Diagonal Stripes (Mac OS Tiger style)
            if (!effectively_full)
            {
                // Use higher contrast stripes if indeterminate (like LoadingBar)
                float stripe_alpha = show_indeterminate ? 0.35f : 0.15f;
                gc.setColor(Color(1, 1, 1, stripe_alpha));
                
                int stripe_spacing = 25;
                for (float dx = -m_height * 2 + m_animation_offset; dx < progress_width + m_height;
                     dx += (float)stripe_spacing)
                {
                    gc.drawLine(m_x + (int)dx, m_y + m_height + 5, m_x + (int)dx + m_height + 5, m_y - 5, 8.0f);
                }
            }

            // 3. Double Gloss / Glassy Effect
            // Top highlight
            gc.fillLinearGradientRect(m_x, m_y, progress_width, m_height / 2, Color(1, 1, 1, 0.4f),
                                      Color(1, 1, 1, 0.05f), true, 0);

            // Bottom "glow"
            gc.fillLinearGradientRect(m_x, m_y + m_height * 0.7f, progress_width, m_height * 0.3f,
                                      Color(1, 1, 1, 0.0f), Color(1, 1, 1, 0.2f), true, 0);

            gc.restore();

            // Crisp border for the progress part
            gc.setColor(c2.darker(0.4f).with_alpha(0.6f));
            gc.drawRect(m_x, m_y, progress_width, m_height, radius, 1.0f);
        }
    }

    void ProgressBar::set_progress(float progress)
    {
        m_target_progress = std::clamp(progress, 0.0f, 1.0f);
        m_is_indeterminate = false;
        invalidate();
    }

    void ProgressBar::set_indeterminate(bool indeterminate)
    {
        m_is_indeterminate = indeterminate;
        invalidate();
    }

    bool ProgressBar::is_indeterminate() const
    {
        return m_is_indeterminate;
    }

    float ProgressBar::progress() const
    {
        return m_progress;
    }
} // namespace horizon
