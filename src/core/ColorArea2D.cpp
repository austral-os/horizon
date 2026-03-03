#include <algorithm>
#include <horizon/ColorArea2D.hpp>
#include <horizon/GraphicsContext.hpp>

namespace horizon
{
    ColorArea2D::ColorArea2D()
    {
        set_size(200, 200);
        when_mouse_press.connect([this](EventContext &ev) { handle_mouse_press(ev); });
        when_mouse_drag.connect([this](EventContext &ev) { handle_mouse_drag(ev); });
        when_mouse_release.connect([this](EventContext &) { m_dragging = false; });
    }

    ColorArea2D::~ColorArea2D() {}

    void ColorArea2D::set_hue(float hue)
    {
        m_hue = std::clamp(hue, 0.0f, 1.0f);
        invalidate();
    }

    void ColorArea2D::set_values(float x_val, float y_val)
    {
        m_val_x = std::clamp(x_val, 0.0f, 1.0f);
        m_val_y = std::clamp(y_val, 0.0f, 1.0f);
        invalidate();
    }

    void ColorArea2D::draw(GraphicsContext &gc)
    {
        int x = m_x;
        int y = m_y;
        int w = m_width;
        int h = m_height;

        // 1. Base Horizontal Gradient: White (S=0) to Hue (S=1)
        Color hue_color(m_hue * 360.0f, 1.0f, 1.0f, true);
        gc.fillLinearGradientRect(x, y, w, h, Color(1.0f, 1.0f, 1.0f), hue_color, false);

        // 2. Vertical Overlay Gradient: Transparent (V=1) to Black (V=0)
        // A vertical gradient from Transparent (0 alpha) to Black (1 alpha)
        gc.fillLinearGradientRect(x, y, w, h, Color(0.0f, 0.0f, 0.0f, 0.0f),
                                  Color(0.0f, 0.0f, 0.0f, 1.0f), true);

        // Selection crosshair
        int px = x + (int)(m_val_x * w);
        int py = y + (int)(m_val_y * h);

        gc.setColor(1.0f, 1.0f, 1.0f);
        gc.drawLine(px - 10, py, px + 10, py, 1.0f);
        gc.drawLine(px, py - 10, px, py + 10, 1.0f);
        gc.setColor(0.0f, 0.0f, 0.0f);
        gc.drawCircle(px, py, 5, 1.0f);
        gc.setColor(1.0f, 1.0f, 1.0f);
        gc.drawCircle(px, py, 6, 1.0f);

        // Border
        gc.setColor(0.1f, 0.1f, 0.1f, 0.8f);
        gc.drawRect(x, y, w, h, 0, 1.0f);
    }

    void ColorArea2D::handle_mouse_press(EventContext &ev)
    {
        if (ev.button == 0x110)
        {
            m_dragging = true;
            update_values_from_pos(ev.eventX, ev.eventY);
        }
    }

    void ColorArea2D::handle_mouse_drag(EventContext &ev)
    {
        if (m_dragging)
        {
            update_values_from_pos(ev.eventX, ev.eventY);
        }
    }

    void ColorArea2D::update_values_from_pos(int x, int y)
    {
        // x,y are window-absolute. Subtract widget's position for local coords.
        float local_x = (float)(x - m_x);
        float local_y = (float)(y - m_y);
        m_val_x = std::clamp(local_x / (float)m_width, 0.0f, 1.0f);
        m_val_y = std::clamp(local_y / (float)m_height, 0.0f, 1.0f);

        EventContext ev;
        ev.data = this;
        when_values_changed.run(ev);
        invalidate();
    }
} // namespace horizon
