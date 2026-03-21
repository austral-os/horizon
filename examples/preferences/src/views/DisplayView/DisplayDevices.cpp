#include <views/DisplayView/DisplayDevices.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <algorithm>
#include <cmath>

namespace horizon::preferences
{
    DisplayDevices::DisplayDevices() : Widget()
    {
        set_background_color(Color(0.1f, 0.1f, 0.12f, 1.0f));
        set_border_radius(8);
        set_border_width(1);
        set_border_color(Color(0.3f, 0.3f, 0.35f, 1.0f));

        refresh_monitors();

        when_mouse_press.connect([this](MouseButtonEventContext &ev) {
            int idx = get_monitor_at((int)ev.x, (int)ev.y);
            if (idx != -1)
            {
                m_dragging_idx = idx;
                m_drag_start_x = (int)ev.x;
                m_drag_start_y = (int)ev.y;
                m_drag_offset_x = m_monitors[idx].info.x;
                m_drag_offset_y = m_monitors[idx].info.y;
                invalidate();
            }
        });

        when_mouse_drag.connect([this](MouseMoveEventContext &ev) {
            if (m_dragging_idx != -1)
            {
                int dx = (int)ev.x - m_drag_start_x;
                int dy = (int)ev.y - m_drag_start_y;

                // Adjust by scale for the drag to feel natural
                m_monitors[m_dragging_idx].info.x = m_drag_offset_x + (int)(dx / m_scale);
                m_monitors[m_dragging_idx].info.y = m_drag_offset_y + (int)(dy / m_scale);

                update_render_rects();
                invalidate();
            }
        });

        when_mouse_release.connect([this](MouseButtonEventContext &) {
            m_dragging_idx = -1;
            invalidate();
        });

        when_mouse_move.connect([this](MouseMoveEventContext &ev) {
            int idx = get_monitor_at((int)ev.x, (int)ev.y);
            bool changed = false;
            for (int i = 0; i < (int)m_monitors.size(); ++i)
            {
                bool h = (i == idx);
                if (m_monitors[i].hovered != h)
                {
                    m_monitors[i].hovered = h;
                    changed = true;
                }
            }
            if (changed)
                invalidate();
        });
    }

    void DisplayDevices::refresh_monitors()
    {
        auto infos = SystemInfo::get_monitors();
        m_monitors.clear();
        for (const auto &info : infos)
        {
            m_monitors.push_back({info, 0, 0, 0, 0, false});
        }
        update_render_rects();
    }

    void DisplayDevices::update_render_rects()
    {
        if (m_monitors.empty())
            return;

        // Calculate bounding box of all monitors
        int min_x = m_monitors[0].info.x;
        int min_y = m_monitors[0].info.y;
        int max_x = m_monitors[0].info.x + m_monitors[0].info.width;
        int max_y = m_monitors[0].info.y + m_monitors[0].info.height;

        for (const auto &m : m_monitors)
        {
            min_x = std::min(min_x, m.info.x);
            min_y = std::min(min_y, m.info.y);
            max_x = std::max(max_x, m.info.x + m.info.width);
            max_y = std::max(max_y, m.info.y + m.info.height);
        }

        int total_w = max_x - min_x;
        int total_h = max_y - min_y;

        // Fit into our widget area (with some padding)
        int pad = 40;
        int avail_w = m_width - pad * 2;
        int avail_h = m_height - pad * 2;

        if (avail_w <= 0 || avail_h <= 0)
            return;

        float scale_w = (float)avail_w / (float)total_w;
        float scale_h = (float)avail_h / (float)total_h;
        m_scale = std::min(scale_w, scale_h);
        
        // Clamp scale to something reasonable if only one monitor
        if (m_monitors.size() == 1) {
             m_scale = std::min(m_scale, 0.2f); // Don't make it TOO big
        }

        m_offset_x = m_x + (m_width - (int)(total_w * m_scale)) / 2 - (int)(min_x * m_scale);
        m_offset_y = m_y + (m_height - (int)(total_h * m_scale)) / 2 - (int)(min_y * m_scale);

        for (auto &m : m_monitors)
        {
            m.rx = m_offset_x + (int)(m.info.x * m_scale);
            m.ry = m_offset_y + (int)(m.info.y * m_scale);
            m.rw = (int)(m.info.width * m_scale);
            m.rh = (int)(m.info.height * m_scale);
        }
    }

    int DisplayDevices::get_monitor_at(int x, int y)
    {
        for (int i = 0; i < (int)m_monitors.size(); ++i)
        {
            const auto &m = m_monitors[i];
            if (x >= m.rx && x <= m.rx + m.rw && y >= m.ry && y <= m.ry + m.rh)
            {
                return i;
            }
        }
        return -1;
    }

    void DisplayDevices::calculate_layout()
    {
        Widget::calculate_layout();
        update_render_rects();
    }

    void DisplayDevices::draw(GraphicsContext &gc)
    {
        // Draw background
        gc.setColor(m_background_color);
        gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(m_border_radius));
        
        gc.setColor(m_border_color);
        gc.drawRect(m_x, m_y, m_width, m_height, CornerRadius(m_border_radius), (float)m_border_width);

        // Draw monitors
        for (int i = 0; i < (int)m_monitors.size(); ++i)
        {
            const auto &m = m_monitors[i];
            
            // Monitor Body
            Color body_color = (m_dragging_idx == i) ? Color(0.25f, 0.45f, 0.85f, 1.0f) : 
                               (m.hovered ? Color(0.25f, 0.25f, 0.30f, 1.0f) : Color(0.20f, 0.20f, 0.25f, 1.0f));
            
            gc.setColor(body_color);
            gc.fillRect(m.rx, m.ry, m.rw, m.rh, CornerRadius(4));
            
            // Border
            Color border_color = (m_dragging_idx == i) ? Color(0.4f, 0.7f, 1.0f, 1.0f) : Color(0.4f, 0.4f, 0.5f, 1.0f);
            gc.setColor(border_color);
            gc.drawRect(m.rx, m.ry, m.rw, m.rh, CornerRadius(4), 2.0f);

            // Name/Model Label
            gc.setColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
            std::string label = m.info.model;
            if (label == "Generic Monitor") label = m.info.conn_name;

            int font_size = std::max(10, (int)(12 * m_scale * 5)); // Scaled font
            font_size = std::min(font_size, 14);

            gc.setDrawFont("sans-serif", font_size, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
            auto metrics = gc.getTextMetrics(label.c_str(), "sans-serif", font_size, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
            
            int tx = m.rx + (m.rw - metrics.width) / 2;
            int ty = m.ry + (m.rh + metrics.height) / 2; // Cairo draws from baseline
            
            if (metrics.width < m.rw - 10) {
                gc.drawText(tx, ty, label.c_str());
            }

            // Draw resolution below
            std::string res = std::to_string(m.info.width) + "x" + std::to_string(m.info.height);
            int res_font_size = font_size - 2;
            gc.setDrawFont("sans-serif", res_font_size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            auto res_metrics = gc.getTextMetrics(res.c_str(), "sans-serif", res_font_size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            
            int rtx = m.rx + (m.rw - res_metrics.width) / 2;
            int rty = ty + res_metrics.height + 4;
            
            if (res_metrics.width < m.rw - 10 && rty < m.ry + m.rh - 5) {
                gc.drawText(rtx, rty, res.c_str());
            }
        }
    }
}
