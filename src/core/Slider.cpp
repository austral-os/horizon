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
        set_fixed_size(40);

        m_thumb_poly = std::make_unique<AquaPolygon>();
        m_thumb_poly->set_accent_color(WidgetAccentColor::Primary);
        m_thumb_poly->set_has_border(true);
        m_thumb_poly->set_border_size(1.0f);

        m_second_thumb_poly = std::make_unique<AquaPolygon>();
        m_second_thumb_poly->set_accent_color(WidgetAccentColor::Primary);
        m_second_thumb_poly->set_has_border(true);
        m_second_thumb_poly->set_border_size(1.0f);

        when_mouse_press.connect([this](MouseButtonEventContext &ev) { handle_mouse_press(ev); });
        when_mouse_drag.connect([this](MouseMoveEventContext &ev) { handle_mouse_drag(ev); });
        when_mouse_release.connect([this](MouseButtonEventContext &) { 
            if (m_dragging_first || m_dragging_second) {
                m_dragging_first = false; 
                m_dragging_second = false;
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
        if (m_second_thumb_poly)
            m_second_thumb_poly->set_application_recursive(app);
    }

    void Slider::set_value(float v)
    {
        float upper_limit = m_enable_range ? m_second_value : m_max;
        v = std::max(m_min, std::min(upper_limit, v));
        if (v != m_value)
        {
            m_value = v;
            update_thumb_polygons();
            EventContext ev;
            ev.sender = this;
            when_value_changed.run(ev);
            invalidate();
        }
    }

    float Slider::value() const
    {
        return m_value;
    }

    void Slider::set_second_value(float v)
    {
        if (!m_enable_range) return;
        v = std::max(m_value, std::min(m_max, v));
        if (v != m_second_value)
        {
            m_second_value = v;
            update_thumb_polygons();
            EventContext ev;
            ev.sender = this;
            when_value_changed.run(ev);
            invalidate();
        }
    }

    float Slider::second_value() const
    {
        return m_second_value;
    }

    void Slider::set_enable_range(bool enable)
    {
        if (m_enable_range != enable)
        {
            m_enable_range = enable;
            if (m_enable_range && m_second_value < m_value) {
                m_second_value = m_value;
            }
            update_thumb_polygons();
            invalidate();
        }
    }

    bool Slider::range_enabled() const
    {
        return m_enable_range;
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
        update_thumb_polygons();
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
        update_thumb_polygons();
        invalidate();
    }

    ThumbShape Slider::thumb_shape() const
    {
        return m_thumb_shape;
    }

    int Slider::thumb_center(float val) const
    {
        float t = (m_max > m_min) ? (val - m_min) / (m_max - m_min) : 0.0f;
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

    void Slider::update_thumb_polygons()
    {
        auto update_poly = [this](std::unique_ptr<AquaPolygon>& poly, float val) {
            if (!poly) return;
            const bool horiz = (m_orientation == SliderOrientation::Horizontal);
            int tc = thumb_center(val);

            std::vector<PolygonPoint> pts;

            if (m_thumb_shape == ThumbShape::Marker)
            {
                if (horiz)
                {
                    // Points DOWN
                    int tx = tc - THUMB_W / 2;
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

                if (horiz)
                {
                    int ticks_area = (m_tick_count > 0 && m_show_ticks) ? (TICK_H + 6) : 0;
                    int usable_h = m_height - ticks_area;
                    int track_y = m_y + (usable_h - TRACK_H) / 2;
                    int tx = tc;
                    int ty = track_y + TRACK_H / 2;

                    float angle_step = PI / 2.0;
                    for (int i = 0; i < 4; ++i)
                    {
                        float angle = i * angle_step;
                        pts.push_back({(int)(tx + r * std::cos(angle)), (int)(ty + r * std::sin(angle)), r});
                    }
                }
                else
                {
                    int ticks_area = (m_tick_count > 0 && m_show_ticks) ? (TICK_H + 6) : 0;
                    int usable_w = m_width - ticks_area;
                    int track_x = m_x + (usable_w - TRACK_H) / 2;
                    int tx = track_x + TRACK_H / 2;
                    int ty = tc;

                    float angle_step = PI / 2.0;
                    for (int i = 0; i < 4; ++i)
                    {
                        float angle = i * angle_step;
                        pts.push_back({(int)(tx + r * std::cos(angle)), (int)(ty + r * std::sin(angle)), r});
                    }
                }
            }
            poly->set_points(pts);
        };

        update_poly(m_thumb_poly, m_value);
        if (m_enable_range)
            update_poly(m_second_thumb_poly, m_second_value);
    }

    void Slider::update_value_from_pos(int x, int y)
    {
        float t;
        int track_len = 0;
        int cursor_px = 0;

        if (m_orientation == SliderOrientation::Horizontal)
        {
            int track_start = m_x + TRACK_PAD;
            int track_end = m_x + m_width - TRACK_PAD;
            track_len = track_end - track_start;
            if (track_len <= 0) return;
            cursor_px = x - track_start;
            t = (float)cursor_px / (float)track_len;
        }
        else
        {
            int track_start = m_y + TRACK_PAD;
            int track_end = m_y + m_height - TRACK_PAD;
            track_len = track_end - track_start;
            if (track_len <= 0) return;
            cursor_px = y - track_start;
            t = 1.0f - (float)cursor_px / (float)track_len;
        }
        t = std::max(0.0f, std::min(1.0f, t));

        // Snap logic
        static constexpr int SNAP_PX = 10;
        if (m_tick_count > 1)
        {
            int px = (m_orientation == SliderOrientation::Horizontal) ? cursor_px : (track_len - cursor_px);
            for (int i = 0; i < m_tick_count; ++i)
            {
                float t_i = (float)i / (float)(m_tick_count - 1);
                int tick_px = (int)(t_i * track_len);
                if (std::abs(px - tick_px) <= SNAP_PX) { t = t_i; break; }
            }
        }
        if (!m_custom_ticks.empty())
        {
            for (float tick_val : m_custom_ticks)
            {
                float t_i = (m_max > m_min) ? (tick_val - m_min) / (m_max - m_min) : 0.0f;
                int tick_px = (int)(t_i * track_len);
                int px = (m_orientation == SliderOrientation::Horizontal) ? cursor_px : (track_len - cursor_px);
                if (std::abs(px - tick_px) <= SNAP_PX) { t = t_i; break; }
            }
        }

        float val = m_min + t * (m_max - m_min);
        if (m_dragging_first) set_value(val);
        else if (m_dragging_second) set_second_value(val);
    }

    void Slider::handle_mouse_press(MouseButtonEventContext &ev)
    {
        if (m_enable_range)
        {
            int c1 = thumb_center(m_value);
            int c2 = thumb_center(m_second_value);
            int pos = (m_orientation == SliderOrientation::Horizontal) ? (int)ev.x : (int)ev.y;
            if (std::abs(pos - c1) < std::abs(pos - c2))
            {
                m_dragging_first = true;
                m_dragging_second = false;
            }
            else
            {
                m_dragging_first = false;
                m_dragging_second = true;
            }
        }
        else
        {
            m_dragging_first = true;
            m_dragging_second = false;
        }
        update_value_from_pos((int)ev.x, (int)ev.y);
    }

    void Slider::handle_mouse_drag(MouseMoveEventContext &ev)
    {
        if (m_dragging_first || m_dragging_second)
            update_value_from_pos((int)ev.x, (int)ev.y);
    }

    void Slider::draw(GraphicsContext &gc)
    {
        const bool horiz = (m_orientation == SliderOrientation::Horizontal);
        int track_x, track_y, track_w, track_h;

        if (horiz)
        {
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

        int track_r = std::min(track_w, track_h) / 2;
        gc.fillLinearGradientRect(track_x, track_y, track_w, track_h,
                                  Color(0.40f, 0.40f, 0.42f, 1.0f),
                                  Color(0.70f, 0.70f, 0.72f, 1.0f), !horiz, CornerRadius(track_r));

        gc.setColor(Color(0.90f, 0.90f, 0.92f, 0.8f));
        if (horiz) gc.fillRect(track_x + track_r, track_y, track_w - track_r * 2, 1);
        else gc.fillRect(track_x, track_y + track_r, 1, track_h - track_r * 2);

        gc.drawLinearGradientRect(
            track_x, track_y, track_w, track_h, Color(0.28f, 0.28f, 0.30f, 1.0f),
            Color(0.55f, 0.55f, 0.58f, 1.0f), 1.0f, !horiz, CornerRadius(track_r));

        auto draw_ticks = [&](const std::vector<float>& ticks, int count) {
            if (count <= 0 && ticks.empty()) return;
            gc.setColor(Color(0.5f, 0.5f, 0.5f, 0.9f));
            if (horiz) {
                int tick_y = track_y + track_h + 5;
                if (ticks.empty()) {
                    for (int i = 0; i < count; ++i) {
                        float t_i = (float)i / (float)(count - 1);
                        gc.fillRect(track_x + (int)(t_i * track_w), tick_y, TICK_W, TICK_H);
                    }
                } else {
                    for (float v : ticks) {
                        float t_i = (m_max > m_min) ? (v - m_min) / (m_max - m_min) : 0.0f;
                        gc.fillRect(track_x + (int)(t_i * track_w), tick_y, TICK_W, TICK_H);
                    }
                }
            } else {
                int tick_x = track_x + track_w + 5;
                if (ticks.empty()) {
                    for (int i = 0; i < count; ++i) {
                        float t_i = (float)i / (float)(count - 1);
                        gc.fillRect(tick_x, track_y + (int)((1.0f - t_i) * track_h), TICK_H, TICK_W);
                    }
                } else {
                    for (float v : ticks) {
                        float t_i = (m_max > m_min) ? (v - m_min) / (m_max - m_min) : 0.0f;
                        gc.fillRect(tick_x, track_y + (int)((1.0f - t_i) * track_h), TICK_H, TICK_W);
                    }
                }
            }
        };

        if (m_show_ticks) {
            draw_ticks({}, m_tick_count);
            draw_ticks(m_custom_ticks, (int)m_custom_ticks.size());
        }

        update_thumb_polygons();
        m_thumb_poly->draw(gc);
        if (m_enable_range)
            m_second_thumb_poly->draw(gc);
    }

} // namespace horizon
