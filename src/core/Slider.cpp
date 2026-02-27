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
        // For horizontal: track is a thin bar vertically centred (above tick area)
        // For vertical:   track is a thin bar horizontally centred

        int track_x, track_y, track_w, track_h;
        int center_x = m_x + m_width / 2;
        int center_y = m_y + m_height / 2;

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
        // Outer sunken fill — dark grey bottom, lighter top (inner shadow illusion)
        gc.fillLinearGradientRect(
            track_x, track_y, track_w, track_h, Color(0.40f, 0.40f, 0.42f, 1.0f),
            Color(0.70f, 0.70f, 0.72f, 1.0f), true, CornerRadius(track_h / 2));

        // Bright highlight at absolute top (1px)
        gc.setColor(Color(0.90f, 0.90f, 0.92f, 0.8f));
        gc.fillRect(track_x + track_h / 2, track_y, track_w - track_h, 1);

        // Track border
        gc.drawLinearGradientRect(
            track_x, track_y, track_w, track_h, Color(0.28f, 0.28f, 0.30f, 1.0f),
            Color(0.55f, 0.55f, 0.58f, 1.0f), 1.0f, true, CornerRadius(track_h / 2));

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

        int tx, ty; // top-left of thumb bounding box
        if (horiz)
        {
            tx = tc - THUMB_W / 2;
            ty = track_y + track_h / 2 - THUMB_H / 2;
        }
        else
        {
            tx = track_x + track_w / 2 - THUMB_W / 2;
            ty = tc - THUMB_H / 2;
        }

        // Shadow
        gc.setColor(Color(0.0f, 0.0f, 0.0f, 0.25f));
        gc.fillRect(tx + 2, ty + THUMB_H - 4, THUMB_W - 2, 6, CornerRadius(3));

        // Thumb body: blue Aqua gradient (top highlight → deep blue bottom)
        Color top_col(0.60f, 0.78f, 1.00f, 1.0f);
        Color bot_col(0.08f, 0.38f, 0.85f, 1.0f);

        // Main body (pill upper part)
        int pill_h = THUMB_H - 8; // flat-bottomed diamond; pointed tip is drawn separately
        gc.fillLinearGradientRect(tx, ty, THUMB_W, pill_h, top_col, bot_col, true,
                                  CornerRadius(THUMB_W / 2, THUMB_W / 2, 0, 0));

        // Pointed bottom — simple filled triangle using fillRect trick: draw shrinking rects
        {
            int base_y = ty + pill_h;
            int steps = 8;
            for (int s = 0; s < steps; ++s)
            {
                float ratio = (float)s / (float)steps;
                float inv = 1.0f - ratio;
                int rect_w = (int)(THUMB_W * inv);
                int rect_x = tx + (THUMB_W - rect_w) / 2;
                // Blend gradient
                Color c(bot_col.r * inv + 0.04f * ratio, bot_col.g * inv + 0.22f * ratio,
                        bot_col.b * inv + 0.50f * ratio, 1.0f);
                gc.setColor(c);
                gc.fillRect(rect_x, base_y + s, rect_w, 1);
            }
        }

        // Inner top gloss (semi-transparent white)
        gc.fillLinearGradientRect(tx + 3, ty + 2, THUMB_W - 6, pill_h / 2,
                                  Color(1.0f, 1.0f, 1.0f, 0.55f), Color(1.0f, 1.0f, 1.0f, 0.0f),
                                  true, CornerRadius(THUMB_W / 2, THUMB_W / 2, 0, 0));

        // Thumb border (pill / rounded top)
        gc.drawLinearGradientRect(tx, ty, THUMB_W, pill_h, Color(0.20f, 0.45f, 0.85f, 1.0f),
                                  Color(0.05f, 0.22f, 0.65f, 1.0f), 1.5f, true,
                                  CornerRadius(THUMB_W / 2, THUMB_W / 2, 0, 0));

        // Erase the unwanted flat bottom stroke of the pill border by overpainting with fill color
        gc.setColor(bot_col);
        gc.fillRect(tx + 1, ty + pill_h - 2, THUMB_W - 2, 4);

        // Pointed-tip border: two diagonal lines from the pill's bottom corners to the tip
        {
            Color tip_border(0.05f, 0.22f, 0.65f, 1.0f);
            gc.setColor(tip_border);
            int tip_steps = 8;
            int base_y = ty + pill_h;
            int tip_x = tx + THUMB_W / 2;
            int tip_y = base_y + tip_steps - 1;
            // Left side: from (tx, base_y) → (tip_x, tip_y)
            gc.drawLine(tx, base_y, tip_x, tip_y, 1.5f);
            // Right side: from (tx + THUMB_W, base_y) → (tip_x, tip_y)
            gc.drawLine(tx + THUMB_W, base_y, tip_x, tip_y, 1.5f);
        }
    }

} // namespace horizon
