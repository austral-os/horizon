#include <algorithm>
#include <horizon/GradientBar.hpp>
#include <horizon/GraphicsContext.hpp>

namespace horizon
{
    GradientBar::GradientBar()
    {
        m_stops = {Color(1.0f, 0.0f, 0.0f), Color(1.0f, 1.0f, 1.0f)}; // Default Red to White
        set_size(200, 20);

        when_mouse_press.connect([this](EventContext &ev) { handle_mouse_press(ev); });
        when_mouse_move.connect([this](EventContext &ev) { handle_mouse_drag(ev); });
        when_mouse_release.connect([this](EventContext &) { m_dragging = false; });
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

            gc.setColor(1.0f, 1.0f, 1.0f, 0.9f);
            if (m_vertical)
            {
                // Simple horizontal line for now, or triangle markers
                gc.drawLine(x - 2, marker_pos, x + w + 2, marker_pos, 2.0f);
            }
            else
            {
                // Triangle markers top and bottom
                std::vector<PolygonPoint> top_tri = {{(int)marker_pos, y - 2, 0},
                                                     {(int)marker_pos - 4, y - 6, 0},
                                                     {(int)marker_pos + 4, y - 6, 0}};
                std::vector<PolygonPoint> bot_tri = {{(int)marker_pos, y + h + 2, 0},
                                                     {(int)marker_pos - 4, y + h + 6, 0},
                                                     {(int)marker_pos + 4, y + h + 6, 0}};

                gc.setColor(0.0f, 0.0f, 0.0f);
                gc.fillPolygon(top_tri);
                gc.fillPolygon(bot_tri);
                gc.setColor(1.0f, 1.0f, 1.0f);
                gc.drawPolygon(top_tri, 1.0f);
                gc.drawPolygon(bot_tri, 1.0f);
            }
        }
    }

    void GradientBar::handle_mouse_press(EventContext &ev)
    {
        if (ev.button == 0x110) // Left click
        {
            m_dragging = true;
            update_value_from_pos(ev.eventX, ev.eventY);
        }
    }

    void GradientBar::handle_mouse_drag(EventContext &ev)
    {
        if (m_dragging)
        {
            update_value_from_pos(ev.eventX, ev.eventY);
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
            ev.data = &m_value;
            when_value_changed.run(ev);
            invalidate();
        }
    }
} // namespace horizon
