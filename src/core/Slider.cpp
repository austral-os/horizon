#include "horizon/WaylandWindow.hpp"
#include <cmath>
#include <vector>
#include <horizon/Application.hpp>
#include <horizon/AquaPolygon.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Slider.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    // Track padding on each side of the widget (so the thumb doesn't clip the edge)
    static constexpr int TRACK_PAD = 12;
    // Track height for horizontal slider (groove thickness)
    static constexpr int TRACK_H = 5;
    // Thumb size in pixels (width of diamond at widest)
    static constexpr int THUMB_W = 22;
    static constexpr int THUMB_H = 28;
    // Tick mark dimensions
    static constexpr int TICK_H = 5;
    static constexpr int TICK_W = 1;
    static constexpr float PI = 3.14159265358979323846f;

    Slider::Slider() : Widget()
    {
        set_focusable(true);
        m_height = 40; // default fixed height for horizontal

        m_thumb_poly = std::make_unique<AquaPolygon>();
        m_thumb_poly->set_accent_color(WidgetAccentColor::Primary);
        m_thumb_poly->set_has_border(true);
        m_thumb_poly->set_border_size(1.0f);

        when_mouse_press.connect([this](MouseButtonEventContext &ev) { handle_mouse_press(ev); });
        when_mouse_drag.connect([this](MouseMoveEventContext &ev) { handle_mouse_drag(ev); });
        when_mouse_release.connect([this](MouseButtonEventContext &) { 
            if (m_dragging) {
                m_dragging = false; 
                EventContext ev;
                ev.sender = this;
                when_changed.run(ev);
            }
        });
    }

    Slider::~Slider() = default;

    void Slider::set_application_recursive(WaylandWindow *app)
    {
        Widget::set_application_recursive(app);
        if (m_thumb_poly)
            m_thumb_poly->set_application_recursive(app);
    }

    void Slider::set_value(float v)
    {
        v = std::max(m_min, std::min(m_max, v));
        if (v != m_value)
        {
            m_value = v;
            update_thumb_polygon();
            EventContext ev;
            ev.sender = this;
            // Since we don't have a specific ValueChangedEventContext yet,
            // we use the base and the user can check the sender.
            when_value_changed.run(ev);
            invalidate();
        }
    }

    float Slider::value() const
    {
        return m_value;
    }

    void Slider::set_min(float min)
    {
        m_min = min;
        invalidate();
    }
    void Slider::set_max(float max)
    {
        m_max = max;
        invalidate();
    }

    void Slider::set_orientation(SliderOrientation o)
    {
        m_orientation = o;
        update_thumb_polygon();
        invalidate();
    }

    SliderOrientation Slider::orientation() const
    {
        return m_orientation;
    }

    void Slider::set_tick_count(int count)
    {
        m_tick_count = count;
        invalidate();
    }

    void Slider::set_show_ticks(bool show)
    {
        m_show_ticks = show;
        invalidate();
    }

    void Slider::add_custom_tick(float v)
    {
        m_custom_ticks.push_back(v);
        invalidate();
    }

    void Slider::clear_custom_ticks()
    {
        m_custom_ticks.clear();
        invalidate();
    }

    bool Slider::show_ticks() const
    {
        return m_show_ticks;
    }

    void Slider::set_thumb_shape(ThumbShape shape)
    {
        m_thumb_shape = shape;
        update_thumb_polygon();
        invalidate();
    }

    ThumbShape Slider::thumb_shape() const
    {
        return m_thumb_shape;
    }

    int Slider::thumb_center() const
    {
        float t = (m_max > m_min) ? (m_value - m_min) / (m_max - m_min) : 0.0f;
        if (m_orientation == SliderOrientation::Horizontal)
        {
            int track_start = m_x + TRACK_PAD;
            int track_end = m_x + m_width - TRACK_PAD;
            return track_start + (int)(t * (track_end - track_start));
        }
        else
        {
            int track_start = m_y + TRACK_PAD;
            int track_end = m_y + m_height - TRACK_PAD;
            // vertical: top = max
            return track_end - (int)(t * (track_end - track_start));
        }
    }

    void Slider::update_thumb_polygon()
    {
        if (!m_thumb_poly)
            return;

        const bool horiz = (m_orientation == SliderOrientation::Horizontal);
        int tc = thumb_center();

        std::vector<PolygonPoint> pts;

        if (m_thumb_shape == ThumbShape::Marker)
        {
            if (horiz)
            {
                // Points DOWN
                int tx = tc - THUMB_W / 2;
                // Center vertically on track
                int track_y;
                if (m_tick_count > 0 && m_show_ticks)
                {
                    int ticks_area = TICK_H + 6;
                    int usable_h = m_height - ticks_area;
                    track_y = m_y + (usable_h - TRACK_H) / 2;
                }
                else
                {
                    track_y = m_y + (m_height - TRACK_H) / 2;
                }

                int ty = track_y + TRACK_H / 2 - THUMB_H / 2;

                int pill_h = THUMB_H - 8;
                pts.push_back({tx, ty, THUMB_W / 2});               // Top-left
                pts.push_back({tx + THUMB_W, ty, THUMB_W / 2});     // Top-right
                pts.push_back({tx + THUMB_W, ty + pill_h, 0});      // Bottom-right base
                pts.push_back({tx + THUMB_W / 2, ty + THUMB_H, 0}); // Tip
                pts.push_back({tx, ty + pill_h, 0});                // Bottom-left base
            }
            else
            {
                // Points RIGHT
                int ty = tc - THUMB_W / 2;
                int track_x;
                if (m_tick_count > 0 && m_show_ticks)
                {
                    int ticks_area = TICK_H + 6;
                    int usable_w = m_width - ticks_area;
                    track_x = m_x + (usable_w - TRACK_H) / 2;
                }
                else
                {
                    track_x = m_x + (m_width - TRACK_H) / 2;
                }

                int tx = track_x + TRACK_H / 2 - THUMB_H / 2;

                int pill_w = THUMB_H - 8;
                pts.push_back({tx, ty, THUMB_W / 2});               // Top-left
                pts.push_back({tx + pill_w, ty, 0});                // Top-right base
                pts.push_back({tx + THUMB_H, ty + THUMB_W / 2, 0}); // Tip
                pts.push_back({tx + pill_w, ty + THUMB_W, 0});      // Bottom-right base
                pts.push_back({tx, ty + THUMB_W, THUMB_W / 2});     // Bottom-left
            }
        }
        else // Circle
        {
            int size = std::min(THUMB_W, THUMB_H) * 1.5;
            int r = size / 2;

            int track_x, track_y;
            if (horiz)
            {
                int ticks_area = (m_tick_count > 0 && m_show_ticks) ? (TICK_H + 6) : 0;
                int usable_h = m_height - ticks_area;
                track_y = m_y + (usable_h - TRACK_H) / 2;
                int tx = tc;
                int ty = track_y + TRACK_H / 2;

                // Simple octagon to approximate circle for AquaPolygon
                float angle_step = PI / 2.0;
                for (int i = 0; i < 4; ++i)
                {
                    float angle = i * angle_step;
                    pts.push_back(
                        {(int)(tx + r * std::cos(angle)), (int)(ty + r * std::sin(angle)), r});
                }
            }
            else
            {
                int ticks_area = (m_tick_count > 0 && m_show_ticks) ? (TICK_H + 6) : 0;
                int usable_w = m_width - ticks_area;
                track_x = m_x + (usable_w - TRACK_H) / 2;
                int tx = track_x + TRACK_H / 2;
                int ty = tc;

                float angle_step = M_PI / 2.0;
                for (int i = 0; i < 4; ++i)
                {
                    float angle = i * angle_step;
                    pts.push_back(
                        {(int)(tx + r * std::cos(angle)), (int)(ty + r * std::sin(angle)), r});
                }
            }
        }

        m_thumb_poly->set_points(pts);
    }

    void Slider::update_value_from_pos(int x, int y)
    {
        float t;
        int track_len = 0;
        int cursor_px = 0; // cursor position along the track axis in pixels

        if (m_orientation == SliderOrientation::Horizontal)
        {
            int track_start = m_x + TRACK_PAD;
            int track_end = m_x + m_width - TRACK_PAD;
            track_len = track_end - track_start;
            if (track_len <= 0)
                return;
            cursor_px = x - track_start;
            t = (float)cursor_px / (float)track_len;
        }
        else
        {
            int track_start = m_y + TRACK_PAD;
            int track_end = m_y + m_height - TRACK_PAD;
            track_len = track_end - track_start;
            if (track_len <= 0)
                return;
            cursor_px = y - track_start;
            t = 1.0f - (float)cursor_px / (float)track_len;
        }
        t = std::max(0.0f, std::min(1.0f, t));

        // ── Tick-mark snapping (magnet behaviour) ───────────────────────────
        // Work entirely in pixel space so the snap radius is consistent
        // regardless of the value range.
        static constexpr int SNAP_PX = 10; // pixels within which snapping activates

        if (m_tick_count > 1)
        {
            // For vertical we inverted t above, so convert cursor_px back to
            // a comparable direction by using (track_len - cursor_px) for vert.
            int px = (m_orientation == SliderOrientation::Horizontal)
                         ? cursor_px
                         : (track_len - cursor_px); // ascending = upward

            for (int i = 0; i < m_tick_count; ++i)
            {
                float t_i = (float)i / (float)(m_tick_count - 1);
                int tick_px = (int)(t_i * track_len);
                int dist = std::abs(px - tick_px);
                if (dist <= SNAP_PX)
                {
                    t = t_i; // snap!
                    break;
                }
            }
        }

        // --- Custom tick-mark snapping ---
        if (!m_custom_ticks.empty())
        {
            for (float tick_val : m_custom_ticks)
            {
                float t_i = (m_max > m_min) ? (tick_val - m_min) / (m_max - m_min) : 0.0f;
                int tick_px = (int)(t_i * track_len);
                int px = (m_orientation == SliderOrientation::Horizontal)
                             ? cursor_px
                             : (track_len - cursor_px);
                int dist = std::abs(px - tick_px);
                if (dist <= SNAP_PX)
                {
                    t = t_i; // snap!
                    break;
                }
            }
        }

        set_value(m_min + t * (m_max - m_min));
    }

    void Slider::handle_mouse_press(MouseButtonEventContext &ev)
    {
        m_dragging = true;
        update_value_from_pos((int)ev.x, (int)ev.y);
    }

    void Slider::handle_mouse_drag(MouseMoveEventContext &ev)
    {
        if (m_dragging)
            update_value_from_pos((int)ev.x, (int)ev.y);
    }

    // -----------------------------------------------------------------------
    // Drawing
    // -----------------------------------------------------------------------

    void Slider::draw(GraphicsContext &gc)
    {
        const bool horiz = (m_orientation == SliderOrientation::Horizontal);

        // ── Layout ──────────────────────────────────────────────────────────
        int track_x, track_y, track_w, track_h;

        if (horiz)
        {
            // Leave a few pixels below for tick marks
            int ticks_area = (m_tick_count > 0 && m_show_ticks) ? (TICK_H + 6) : 0;
            int usable_h = m_height - ticks_area;
            track_x = m_x + TRACK_PAD;
            track_y = m_y + (usable_h - TRACK_H) / 2;
            track_w = m_width - TRACK_PAD * 2;
            track_h = TRACK_H;
        }
        else
        {
            int ticks_area = (m_tick_count > 0 && m_show_ticks) ? (TICK_H + 6) : 0;
            int usable_w = m_width - ticks_area;
            track_y = m_y + TRACK_PAD;
            track_x = m_x + (usable_w - TRACK_H) / 2;
            track_w = TRACK_H;
            track_h = m_height - TRACK_PAD * 2;
        }

        // ── 1. Track (recessed groove) ───────────────────────────────────
        // Corner radius = half the THIN side of the track bar (produces pill ends)
        int track_r = std::min(track_w, track_h) / 2;

        // Outer sunken fill — dark → light gradient
        gc.fillLinearGradientRect(track_x, track_y, track_w, track_h,
                                  Color(0.40f, 0.40f, 0.42f, 1.0f),
                                  Color(0.70f, 0.70f, 0.72f, 1.0f), !horiz, CornerRadius(track_r));

        // Bright highlight (1px line inside the top/left edge of the track)
        gc.setColor(Color(0.90f, 0.90f, 0.92f, 0.8f));
        if (horiz)
            gc.fillRect(track_x + track_r, track_y, track_w - track_r * 2, 1);
        else
            gc.fillRect(track_x, track_y + track_r, 1, track_h - track_r * 2);

        // Track border
        gc.drawLinearGradientRect(
            track_x, track_y, track_w, track_h, Color(0.28f, 0.28f, 0.30f, 1.0f),
            Color(0.55f, 0.55f, 0.58f, 1.0f), 1.0f, !horiz, CornerRadius(track_r));

        // ── 2. Tick marks ────────────────────────────────────────────────
        if (m_tick_count > 1 && m_show_ticks)
        {
            gc.setColor(Color(0.5f, 0.5f, 0.5f, 0.9f));
            if (horiz)
            {
                int tick_y = track_y + track_h + 5;
                for (int i = 0; i < m_tick_count; ++i)
                {
                    float t_i = (float)i / (float)(m_tick_count - 1);
                    int tx = track_x + (int)(t_i * track_w);
                    gc.fillRect(tx, tick_y, TICK_W, TICK_H);
                }
            }
            else
            {
                int tick_x = track_x + track_w + 5;
                for (int i = 0; i < m_tick_count; ++i)
                {
                    float t_i = (float)i / (float)(m_tick_count - 1);
                    int ty = track_y + (int)((1.0f - t_i) * track_h);
                    gc.fillRect(tick_x, ty, TICK_H, TICK_W);
                }
            }
        }

        // --- 2b. Custom Tick marks ---
        if (!m_custom_ticks.empty() && m_show_ticks)
        {
            gc.setColor(Color(0.5f, 0.5f, 0.5f, 0.9f));
            if (horiz)
            {
                int tick_y = track_y + track_h + 5;
                for (float tick_val : m_custom_ticks)
                {
                    float t_i = (m_max > m_min) ? (tick_val - m_min) / (m_max - m_min) : 0.0f;
                    int tx = track_x + (int)(t_i * track_w);
                    gc.fillRect(tx, tick_y, TICK_W, TICK_H);
                }
            }
            else
            {
                int tick_x = track_x + track_w + 5;
                for (float tick_val : m_custom_ticks)
                {
                    float t_i = (m_max > m_min) ? (tick_val - m_min) / (m_max - m_min) : 0.0f;
                    int ty = track_y + (int)((1.0f - t_i) * track_h);
                    gc.fillRect(tick_x, ty, TICK_H, TICK_W);
                }
            }
        }

        // ── 3. Thumb (AquaPolygon) ──────────────────────────────────────
        update_thumb_polygon();
        m_thumb_poly->draw(gc);
    }

} // namespace horizon
