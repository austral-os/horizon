#include <cmath>
#include <horizon/Application.hpp>
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

    Slider::Slider() : Widget()
    {
        m_height = 40; // default fixed height for horizontal

        when_mouse_press.connect([this](EventContext &ev) { handle_mouse_press(ev); });
        when_mouse_drag.connect([this](EventContext &ev) { handle_mouse_drag(ev); });
        when_mouse_release.connect([this](EventContext &) { m_dragging = false; });
    }

    void Slider::set_value(float v)
    {
        v = std::max(m_min, std::min(m_max, v));
        if (v != m_value)
        {
            m_value = v;
            if (m_on_value_changed)
                m_on_value_changed(m_value);
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

    void Slider::set_on_value_changed(std::function<void(float)> cb)
    {
        m_on_value_changed = std::move(cb);
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

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

    void Slider::update_value_from_pos(int x, int y)
    {
        float t;
        if (m_orientation == SliderOrientation::Horizontal)
        {
            int track_start = m_x + TRACK_PAD;
            int track_end = m_x + m_width - TRACK_PAD;
            int track_len = track_end - track_start;
            if (track_len <= 0)
                return;
            t = (float)(x - track_start) / (float)track_len;
        }
        else
        {
            int track_start = m_y + TRACK_PAD;
            int track_end = m_y + m_height - TRACK_PAD;
            int track_len = track_end - track_start;
            if (track_len <= 0)
                return;
            t = 1.0f - (float)(y - track_start) / (float)track_len;
        }
        t = std::max(0.0f, std::min(1.0f, t));
        set_value(m_min + t * (m_max - m_min));
    }

    void Slider::handle_mouse_press(EventContext &ev)
    {
        m_dragging = true;
        update_value_from_pos((int)ev.eventX, (int)ev.eventY);
    }

    void Slider::handle_mouse_drag(EventContext &ev)
    {
        if (m_dragging)
            update_value_from_pos((int)ev.eventX, (int)ev.eventY);
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
            int ticks_area = (m_tick_count > 0) ? (TICK_H + 6) : 0;
            int usable_h = m_height - ticks_area;
            track_x = m_x + TRACK_PAD;
            track_y = m_y + (usable_h - TRACK_H) / 2;
            track_w = m_width - TRACK_PAD * 2;
            track_h = TRACK_H;
        }
        else
        {
            int ticks_area = (m_tick_count > 0) ? (TICK_H + 6) : 0;
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
        if (m_tick_count > 1)
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

        // ── 3. Thumb (Aqua diamond / teardrop) ──────────────────────────
        int tc = thumb_center();

        // ── 3.a HORIZONTAL thumb: pill on top, tip pointing DOWN ──────────
        if (horiz)
        {
            int tx = tc - THUMB_W / 2;
            int ty = track_y + track_h / 2 - THUMB_H / 2;

            // Shadow
            gc.setColor(Color(0.0f, 0.0f, 0.0f, 0.22f));
            gc.fillRect(tx + 2, ty + THUMB_H - 3, THUMB_W - 4, 5, CornerRadius(3));

            Color top_col(0.62f, 0.80f, 1.00f, 1.0f);
            Color bot_col(0.08f, 0.38f, 0.85f, 1.0f);

            int pill_h = THUMB_H - 8;

            // Pill fill
            gc.fillLinearGradientRect(tx, ty, THUMB_W, pill_h, top_col, bot_col, true,
                                      CornerRadius(THUMB_W / 2, THUMB_W / 2, 0, 0));

            // Pointed bottom (shrinking horizontal rects downward)
            {
                int base_y = ty + pill_h;
                int steps = 8;
                for (int s = 0; s < steps; ++s)
                {
                    float inv = 1.0f - (float)s / (float)steps;
                    int rect_w = (int)(THUMB_W * inv);
                    int rect_x = tx + (THUMB_W - rect_w) / 2;
                    Color c(bot_col.r * inv + 0.04f * (1 - inv),
                            bot_col.g * inv + 0.22f * (1 - inv),
                            bot_col.b * inv + 0.50f * (1 - inv), 1.0f);
                    gc.setColor(c);
                    gc.fillRect(rect_x, base_y + s, rect_w, 1);
                }
            }

            // Gloss
            gc.fillLinearGradientRect(tx + 3, ty + 2, THUMB_W - 6, pill_h / 2,
                                      Color(1.0f, 1.0f, 1.0f, 0.55f), Color(1.0f, 1.0f, 1.0f, 0.0f),
                                      true, CornerRadius(THUMB_W / 2, THUMB_W / 2, 0, 0));

            // Pill border
            gc.drawLinearGradientRect(tx, ty, THUMB_W, pill_h, Color(0.20f, 0.45f, 0.85f, 1.0f),
                                      Color(0.05f, 0.22f, 0.65f, 1.0f), 1.5f, true,
                                      CornerRadius(THUMB_W / 2, THUMB_W / 2, 0, 0));

            // Erase flat bottom edge of pill border
            gc.setColor(bot_col);
            gc.fillRect(tx + 1, ty + pill_h - 2, THUMB_W - 2, 4);

            // Diagonal tip border
            Color tip_border(0.05f, 0.22f, 0.65f, 1.0f);
            gc.setColor(tip_border);
            int base_y = ty + pill_h;
            int tip_x = tx + THUMB_W / 2;
            int tip_y = base_y + 7;
            gc.drawLine(tx, base_y, tip_x, tip_y, 1.5f);
            gc.drawLine(tx + THUMB_W, base_y, tip_x, tip_y, 1.5f);
        }
        // ── 3.b VERTICAL thumb: pill on left, tip pointing RIGHT ──────────
        else
        {
            // Thumb bounding box: total width = THUMB_H, height = THUMB_W
            // (rotated 90° CW from horizontal)
            int thumb_total_w = THUMB_H; // total extent along track-perpendicular axis
            int thumb_h = THUMB_W;       // extent along track axis
            int tx = track_x + track_w / 2 - thumb_total_w / 2;
            int ty = tc - thumb_h / 2;

            int pill_w = THUMB_H - 8; // width of the pill (left) part

            Color top_col(0.62f, 0.80f, 1.00f, 1.0f); // light blue (left/top of gradient)
            Color bot_col(0.08f, 0.38f, 0.85f, 1.0f); // deep blue  (right/bottom)

            // Shadow
            gc.setColor(Color(0.0f, 0.0f, 0.0f, 0.22f));
            gc.fillRect(tx + thumb_total_w - 3, ty + 2, 5, thumb_h - 4, CornerRadius(3));

            // Pill fill — horizontal gradient (left=light, right=dark), rounded LEFT corners
            gc.fillLinearGradientRect(tx, ty, pill_w, thumb_h, top_col, bot_col, false,
                                      CornerRadius(thumb_h / 2, 0, 0, thumb_h / 2));

            // Pointed tip — shrinking vertical rects extending to the RIGHT
            {
                int base_x = tx + pill_w;
                int steps = 8;
                for (int s = 0; s < steps; ++s)
                {
                    float inv = 1.0f - (float)s / (float)steps;
                    int rect_h = (int)(thumb_h * inv);
                    int rect_y = ty + (thumb_h - rect_h) / 2;
                    Color c(bot_col.r * inv + 0.04f * (1 - inv),
                            bot_col.g * inv + 0.22f * (1 - inv),
                            bot_col.b * inv + 0.50f * (1 - inv), 1.0f);
                    gc.setColor(c);
                    gc.fillRect(base_x + s, rect_y, 1, rect_h);
                }
            }

            // Gloss (left half of pill, top portion)
            gc.fillLinearGradientRect(tx + 2, ty + 3, pill_w / 2, thumb_h - 6,
                                      Color(1.0f, 1.0f, 1.0f, 0.55f), Color(1.0f, 1.0f, 1.0f, 0.0f),
                                      false, CornerRadius(thumb_h / 2, 0, 0, thumb_h / 2));

            // Pill border — rounded LEFT corners
            gc.drawLinearGradientRect(tx, ty, pill_w, thumb_h, Color(0.20f, 0.45f, 0.85f, 1.0f),
                                      Color(0.05f, 0.22f, 0.65f, 1.0f), 1.5f, false,
                                      CornerRadius(thumb_h / 2, 0, 0, thumb_h / 2));

            // Erase flat RIGHT edge of pill border (junction with tip)
            gc.setColor(bot_col);
            gc.fillRect(tx + pill_w - 2, ty + 1, 4, thumb_h - 2);

            // Diagonal tip border lines from pill's right corners to the tip point
            Color tip_border(0.05f, 0.22f, 0.65f, 1.0f);
            gc.setColor(tip_border);
            int base_x = tx + pill_w;
            int tip_x = base_x + 7;
            int tip_y = ty + thumb_h / 2;
            gc.drawLine(base_x, ty, tip_x, tip_y, 1.5f);
            gc.drawLine(base_x, ty + thumb_h, tip_x, tip_y, 1.5f);
        }
    }

} // namespace horizon
