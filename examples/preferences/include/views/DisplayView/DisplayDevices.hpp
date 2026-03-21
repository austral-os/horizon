#pragma once

#include <horizon/Widget.hpp>
#include <horizon/SystemInfo.hpp>
#include <vector>

namespace horizon::preferences
{
    /**
     * @class DisplayDevices
     * @brief A widget that displays connected monitors as draggable rectangles.
     */
    class DisplayDevices : public Widget
    {
    public:
        DisplayDevices();
        ~DisplayDevices() override = default;

        void draw(GraphicsContext &gc) override;
        void calculate_layout() override;

    private:
        struct MonitorRect
        {
            MonitorInfo info;
            int rx, ry, rw, rh; // Render coordinates and size
            bool hovered = false;
        };

        void refresh_monitors();
        void update_render_rects();
        int get_monitor_at(int x, int y);

        std::vector<MonitorRect> m_monitors;
        int m_dragging_idx = -1;
        int m_drag_start_x = 0;
        int m_drag_start_y = 0;
        int m_drag_offset_x = 0;
        int m_drag_offset_y = 0;

        float m_scale = 1.0f;
        int m_offset_x = 0;
        int m_offset_y = 0;
    };
}
