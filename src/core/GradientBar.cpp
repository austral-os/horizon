#include <algorithm>
#include <horizon/GradientBar.hpp>
#include <horizon/GraphicsContext.hpp>

namespace horizon
{
    GradientBar::GradientBar()
    {
        m_stops = {Color(1.0f, 0.0f, 0.0f), Color(1.0f, 1.0f, 1.0f)}; // Default Red to White
        set_size(200, 20);

        when_mouse_press.connect([this](MouseButtonEventContext &ev) { handle_mouse_press(ev); });
        when_mouse_drag.connect([this](MouseMoveEventContext &ev) { handle_mouse_drag(ev); });
        when_mouse_release.connect([this](MouseButtonEventContext &) { m_dragging = false; });
    }

    GradientBar::~GradientBar() {}

    void GradientBar::set_stops(const std::vector<Color> &stops)
    {
        m_stops = stops;
        invalidate();
    }

    void GradientBar::set_value(float value)
    {
        m_value = std::clamp(value, 0.0f, 1.0f);
        invalidate();
    }

    void GradientBar::set_vertical(bool vertical)
    {
        m_vertical = vertical;
        invalidate();
    }

    void GradientBar::draw(GraphicsContext &gc)
    {
        if (m_stops.empty())
            return;

        int x = m_x;
        int y = m_y;
        int w = m_width;
        int h = m_height;

        // Draw background gradient
        if (m_stops.size() == 1)
        {
            gc.setColor(m_stops[0]);
            gc.fillRect(x, y, w, h, 2);
        }
        else
        {
            int num_segments = m_stops.size() - 1;
            for (int i = 0; i < num_segments; ++i)
            {
                int seg_x, seg_y, seg_w, seg_h;
                if (m_vertical)
                {
                    seg_x = x;
                    seg_w = w;
                    seg_y = y + (i * h) / num_segments;
                    seg_h = ((i + 1) * h) / num_segments - (i * h) / num_segments;
                }
                else
                {
                    seg_y = y;
                    seg_h = h;
                    seg_x = x + (i * w) / num_segments;
                    seg_w = ((i + 1) * w) / num_segments - (i * w) / num_segments;
                }

                gc.fillLinearGradientRect(seg_x, seg_y, seg_w, seg_h, m_stops[i], m_stops[i + 1],
                                          m_vertical);
            }
        }

        // Draw border
        gc.setColor(0.1f, 0.1f, 0.1f, 0.5f);
        gc.drawRect(x, y, w, h, 2, 1.0f);

        // Draw marker
        if (m_show_marker)
        {
            int marker_pos;
            if (m_vertical)
                marker_pos = y + (int)(m_value * h);
            else
                marker_pos = x + (int)(m_value * w);

            gc.setColor(0.0f, 0.0f, 0.0f);
            if (m_vertical)
            {
                // Triangle markers left and right
                std::vector<PolygonPoint> left_tri = {{(int)x, marker_pos, 0},
                                                      {(int)x + 5, marker_pos - 5, 0},
                                                      {(int)x + 5, marker_pos + 5, 0}};
                std::vector<PolygonPoint> right_tri = {{(int)x + w, marker_pos, 0},
                                                       {(int)x + w - 5, marker_pos - 5, 0},
                                                       {(int)x + w - 5, marker_pos + 5, 0}};

                gc.fillPolygon(left_tri);
                gc.fillPolygon(right_tri);
                gc.setColor(1.0f, 1.0f, 1.0f);
                gc.drawPolygon(left_tri, 1.0f);
                gc.drawPolygon(right_tri, 1.0f);
            }
            else
            {
                // Triangle markers top and bottom
                std::vector<PolygonPoint> top_tri = {{(int)marker_pos, y, 0},
                                                     {(int)marker_pos - 5, y + 5, 0},
                                                     {(int)marker_pos + 5, y + 5, 0}};
                std::vector<PolygonPoint> bot_tri = {{(int)marker_pos, y + h, 0},
                                                     {(int)marker_pos - 5, y + h - 5, 0},
                                                     {(int)marker_pos + 5, y + h - 5, 0}};

                gc.fillPolygon(top_tri);
                gc.fillPolygon(bot_tri);
                gc.setColor(1.0f, 1.0f, 1.0f);
                gc.drawPolygon(top_tri, 1.0f);
                gc.drawPolygon(bot_tri, 1.0f);
            }
        }
    }

    void GradientBar::handle_mouse_press(MouseButtonEventContext &ev)
    {
        if (ev.button == 0x110) // Left click
        {
            m_dragging = true;
            update_value_from_pos(ev.x, ev.y);
        }
    }

    void GradientBar::handle_mouse_drag(MouseMoveEventContext &ev)
    {
        if (m_dragging)
        {
            update_value_from_pos(ev.x, ev.y);
        }
    }

    void GradientBar::update_value_from_pos(int x, int y)
    {
        float old_value = m_value;
        // x,y are window-absolute. Subtract widget's position for local coords.
        if (m_vertical)
            m_value = std::clamp((float)(y - m_y) / (float)m_height, 0.0f, 1.0f);
        else
            m_value = std::clamp((float)(x - m_x) / (float)m_width, 0.0f, 1.0f);

        if (m_value != old_value)
        {
            EventContext ev;
            ev.sender = this;
            when_value_changed.run(ev);
            invalidate();
        }
    }
} // namespace horizon
