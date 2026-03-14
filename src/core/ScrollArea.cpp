#include "horizon/WaylandWindow.hpp"
#include <algorithm>
#include <cmath>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/TableRow.hpp>

namespace horizon
{
    ScrollArea::ScrollArea() : Widget()
    {
        m_layout_type = WIDGET_LAYOUT_VERTICAL;
        m_position_type = FILL;

        m_h_thumb = std::make_unique<AquaPolygon>();
        m_h_thumb->set_accent_color(WidgetAccentColor::Primary);
        m_h_thumb->set_has_border(true);
        m_h_thumb->set_border_size(1.0f);

        m_v_thumb = std::make_unique<AquaPolygon>();
        m_v_thumb->set_accent_color(WidgetAccentColor::Primary);
        m_v_thumb->set_has_border(true);
        m_v_thumb->set_border_size(1.0f);

        when_mouse_press.connect([this](MouseButtonEventContext &ev) { handle_mouse_press(ev); });
        when_mouse_drag.connect([this](MouseMoveEventContext &ev) { handle_mouse_drag(ev); });
        when_mouse_release.connect([this](MouseButtonEventContext &ev)
                                   { handle_mouse_release(ev); });
        when_mouse_move.connect([this](MouseMoveEventContext &ev) { handle_mouse_move(ev); });

        when_mouse_wheel.connect(
            [this](MouseWheelEventContext &ev)
            {
                // Simple vertical scroll implementation
                // Scroll multiplier: around 30-40 pixels per "notch" (usually dy is ~1.0 or 10.0)
                int scroll_amount = (int)(ev.dy * 4.0f);
                if (std::abs(ev.dy) > 0)
                {
                    set_scroll_position(m_scroll_x, m_scroll_y + scroll_amount);
                }

                // If horizontal scroll is present
                if (std::abs(ev.dx) > 0)
                {
                    int h_scroll_amount = (int)(ev.dx * 4.0f);
                    set_scroll_position(m_scroll_x + h_scroll_amount, m_scroll_y);
                }

                ev.stop_propagation = true;
            });
    }

    ScrollArea::~ScrollArea() = default;

    void ScrollArea::set_content(std::unique_ptr<Widget> child)
    {
        // Enforce single child policy
        m_children.clear();
        if (child)
        {
            add_child(std::move(child));
        }
        invalidate();
    }

    void ScrollArea::set_scroll_position(int x, int y)
    {
        if (m_children.empty())
            return;
        Widget *content = m_children[0].get();

        int max_x = std::max(0, content->width() - m_width);
        int max_y = std::max(0, content->height() - m_height);

        int new_x = std::max(0, std::min(x, max_x));
        int new_y = std::max(0, std::min(y, max_y));

        if (new_x != m_scroll_x || new_y != m_scroll_y)
        {
            m_scroll_x = new_x;
            m_scroll_y = new_y;

            EventContext ev;
            ev.sender = this;
            when_scroll.run(ev);

            invalidate();
        }
    }

    void ScrollArea::calculate_layout()
    {
        Widget::calculate_layout();
        update_scrollbars();
    }

    void ScrollArea::render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force)
    {
        if (!m_visible)
            return;

        bool intersects =
            !(m_x >= cx + cw || m_x + m_width <= cx || m_y >= cy + ch || m_y + m_height <= cy);
        if (!intersects)
            return;

        calculate_layout();

        bool should_draw = m_dirty || force || m_child_dirty;

        // 1. Draw background (optional)
        if (should_draw)
        {
            // gc.setColor(Color(0.95f, 0.95f, 0.95f, 1.0f));
            // gc.fillRect(m_x, m_y, m_width, m_height);
        }

        // 2. Render child with clipping and offset
        if (!m_children.empty())
        {
            Widget *child = m_children[0].get();

            gc.save();
            // Subtract space for scrollbars if they are visible
            int clip_w = m_width - (m_show_v_scroll ? SCROLLBAR_SIZE + 2 : 0);
            int clip_h = m_height - (m_show_h_scroll ? SCROLLBAR_SIZE + 2 : 0);

            gc.clip(m_x, m_y, clip_w, clip_h);

            // Position child with offset
            child->set_position(m_x - m_scroll_x, m_y - m_scroll_y);

            child->render(gc, cx, cy, cw, ch, should_draw);
            gc.restore();
        }

        // 3. Draw scrollbars ON TOP
        if (should_draw)
        {
            draw(gc);
        }

        m_dirty = false;
        m_child_dirty = false;
    }

    void ScrollArea::draw(GraphicsContext &gc)
    {
        // update_scrollbars is now called in render

        if (m_show_v_scroll)
        {
            // Vertical track
            gc.setColor(Color(0.85f, 0.85f, 0.85f, 0.5f));
            gc.fillRect(m_v_track_x, m_v_track_y, m_v_track_w, m_v_track_h,
                        CornerRadius(m_v_track_w / 2));
            m_v_thumb->draw(gc);
        }

        if (m_show_h_scroll)
        {
            // Horizontal track
            gc.setColor(Color(0.85f, 0.85f, 0.85f, 0.5f));
            gc.fillRect(m_h_track_x, m_h_track_y, m_h_track_w, m_h_track_h,
                        CornerRadius(m_h_track_h / 2));
            m_h_thumb->draw(gc);
        }
    }

    void ScrollArea::update_scrollbars()
    {
        if (m_children.empty())
        {
            m_show_h_scroll = false;
            m_show_v_scroll = false;
            return;
        }

        Widget *content = m_children[0].get();
        m_show_h_scroll = content->width() > m_width;
        m_show_v_scroll = content->height() > m_height;

        if (m_show_v_scroll)
        {
            m_v_track_x = m_x + m_width - SCROLLBAR_SIZE - 2;
            m_v_track_y = m_y + 2;
            m_v_track_w = SCROLLBAR_SIZE;
            m_v_track_h = m_height - 4 - (m_show_h_scroll ? SCROLLBAR_SIZE : 0);

            float visible_ratio = (float)m_height / (float)content->height();
            int thumb_h = std::max(20, (int)(m_v_track_h * visible_ratio));
            float scroll_ratio = (float)m_scroll_y / (float)(content->height() - m_height);
            int thumb_y = m_v_track_y + (int)(scroll_ratio * (m_v_track_h - thumb_h));

            std::vector<PolygonPoint> pts;
            int r = m_v_track_w / 2;
            pts.push_back({m_v_track_x, thumb_y, r});
            pts.push_back({m_v_track_x + m_v_track_w, thumb_y, r});
            pts.push_back({m_v_track_x + m_v_track_w, thumb_y + thumb_h, r});
            pts.push_back({m_v_track_x, thumb_y + thumb_h, r});
            m_v_thumb->set_points(pts);
        }

        if (m_show_h_scroll)
        {
            m_h_track_x = m_x + 2;
            m_h_track_y = m_y + m_height - SCROLLBAR_SIZE - 2;
            m_h_track_w = m_width - 4 - (m_show_v_scroll ? SCROLLBAR_SIZE : 0);
            m_h_track_h = SCROLLBAR_SIZE;

            float visible_ratio = (float)m_width / (float)content->width();
            int thumb_w = std::max(20, (int)(m_h_track_w * visible_ratio));
            float scroll_ratio = (float)m_scroll_x / (float)(content->width() - m_width);
            int thumb_x = m_h_track_x + (int)(scroll_ratio * (m_h_track_w - thumb_w));

            std::vector<PolygonPoint> pts;
            int r = m_h_track_h / 2;
            pts.push_back({thumb_x, m_h_track_y, r});
            pts.push_back({thumb_x + thumb_w, m_h_track_y, r});
            pts.push_back({thumb_x + thumb_w, m_h_track_y + m_h_track_h, r});
            pts.push_back({thumb_x, m_h_track_y + m_h_track_h, r});
            m_h_thumb->set_points(pts);
        }
    }

    Widget *ScrollArea::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled)
            return nullptr;

        if (x < m_x || y < m_y || x >= m_x + m_width || y >= m_y + m_height)
            return nullptr;

        // Check scrollbars first
        if (m_show_v_scroll && x >= m_v_track_x && x < m_v_track_x + m_v_track_w &&
            y >= m_v_track_y && y < m_v_track_y + m_v_track_h)
        {
            return this;
        }
        if (m_show_h_scroll && y >= m_h_track_y && y < m_h_track_y + m_h_track_h &&
            x >= m_h_track_x && x < m_h_track_x + m_h_track_w)
        {
            return this;
        }

        // Check child with offset
        if (!m_children.empty())
        {
            Widget *hit = m_children[0]->hit_test(x, y);
            if (hit && hit != m_children[0].get())
                return hit;
            if (hit == m_children[0].get())
                return this; // Return self or child? Usually ScrollArea captures.
        }

        return this;
    }

    void ScrollArea::set_application_recursive(WaylandWindow *app)
    {
        Widget::set_application_recursive(app);
        if (m_h_thumb)
            m_h_thumb->set_application_recursive(app);
        if (m_v_thumb)
            m_v_thumb->set_application_recursive(app);
    }

    void ScrollArea::handle_mouse_press(MouseButtonEventContext &ev)
    {
        if (m_show_v_scroll && ev.x >= m_v_track_x && ev.x < m_v_track_x + m_v_track_w &&
            ev.y >= m_v_track_y && ev.y < m_v_track_y + m_v_track_h)
        {
            m_dragging_v = true;
            m_drag_start_pos = ev.y;
            m_drag_start_scroll = m_scroll_y;
        }
        else if (m_show_h_scroll && ev.y >= m_h_track_y && ev.y < m_h_track_y + m_h_track_h &&
                 ev.x >= m_h_track_x && ev.x < m_h_track_x + m_h_track_w)
        {
            m_dragging_h = true;
            m_drag_start_pos = ev.x;
            m_drag_start_scroll = m_scroll_x;
        }
    }

    void ScrollArea::handle_mouse_drag(MouseMoveEventContext &ev)
    {
        if (m_dragging_v && !m_children.empty())
        {
            int delta = ev.y - m_drag_start_pos;
            Widget *content = m_children[0].get();
            float track_usable =
                m_v_track_h -
                std::max(20, (int)(m_v_track_h * ((float)m_height / content->height())));
            float scroll_max = content->height() - m_height;
            if (track_usable > 0)
            {
                int new_scroll = m_drag_start_scroll + (int)(delta * (scroll_max / track_usable));
                set_scroll_position(m_scroll_x, new_scroll);
            }
        }
        else if (m_dragging_h && !m_children.empty())
        {
            int delta = ev.x - m_drag_start_pos;
            Widget *content = m_children[0].get();
            float track_usable =
                m_h_track_w -
                std::max(20, (int)(m_h_track_w * ((float)m_width / content->width())));
            float scroll_max = content->width() - m_width;
            if (track_usable > 0)
            {
                int new_scroll = m_drag_start_scroll + (int)(delta * (scroll_max / track_usable));
                set_scroll_position(new_scroll, m_scroll_y);
            }
        }
    }

    void ScrollArea::handle_mouse_release(MouseButtonEventContext &)
    {
        m_dragging_v = false;
        m_dragging_h = false;
    }

    void ScrollArea::handle_mouse_move(MouseMoveEventContext &ev) {}

} // namespace horizon
