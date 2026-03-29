#include "DockShelf.hpp"
#include <horizon/GraphicsContext.hpp>
#include <horizon/Icon.hpp>
#include <horizon/WaylandWindow.hpp>
#include <vector>

namespace horizon
{

    DockShelf::DockShelf()
    {
        // Horizontal layout with padding for the shelf appearance
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);
        // Leave space at the bottom for the 3D lip of the shelf
        set_margin(20); // Changed from 10 to 20
        set_size(0, 100);

        when_mouse_move.connect(
            [this](MouseMoveEventContext &ctx)
            {
                if (m_magnification_enabled)
                {
                    widget_position pos = get_absolute_position();
                    int sx = application() ? application()->screen_x() : 0;
                    int sy = application() ? application()->screen_y() : 0;
                    int shelf_x = pos.x - sx;
                    int shelf_y = pos.y - sy;
                    m_mouse_x = (int)ctx.x - shelf_x;
                    m_mouse_y = (int)ctx.y - shelf_y;
                    m_mouse_over = true;
                    invalidate();
                    calculate_layout();
                }
            });

        when_mouse_leave.connect(
            [this](EventContext &ctx)
            {
                if (m_magnification_enabled)
                {
                    m_mouse_over = false;
                    invalidate();
                    calculate_layout();
                }
            });
    }

    void DockShelf::calculate_layout()
    {
        const float radius = 120.0f;

        int total_children_width = 0;
        int count = 0;

        for (const auto &child : children())
        {
            if (child->is_visible())
            {
                Icon *icon_child = dynamic_cast<Icon *>(child.get());
                int current_icon_size = m_base_size;

                if (m_magnification_enabled && m_mouse_over)
                {
                    // 1. Calculate horizontal distance
                    float child_center_x = (float)total_children_width + child->fixed_size() / 2.0f;
                    float dist_x = std::abs((child_center_x + margin()) - m_mouse_x);
                    
                    // 2. Calculate vertical distance from the 'active strip'
                    // The icon strip is centered horizontally around its base position
                    // We'll use the 'seated' bottom position as the reference for Y
                    float total_h = m_base_size * 2.5f;
                    float lip_height = 10.0f;
                    float active_y = (total_h - lip_height - 9) - (m_base_size / 2.0f);
                    float dist_y = std::abs(active_y - m_mouse_y);

                    // 3. Combined 2D Gaussian Scale
                    float radius_y = 80.0f; 
                    float scale_x = std::exp(-(dist_x * dist_x) / (2 * radius * radius));
                    float scale_y = std::exp(-(dist_y * dist_y) / (2 * radius_y * radius_y));
                    float scale = scale_x * scale_y;

                    current_icon_size = m_base_size + (int)(m_max_extra_size * scale);
                    child->set_fixed_size(current_icon_size + child->margin() * 2);
                }
                else
                {
                    child->set_fixed_size(m_base_size + child->margin() * 2);
                }

                if (icon_child)
                {
                    icon_child->set_icon_size(current_icon_size);
                }

                // MANUALLY SET ABSOLUTE POSITION AND SIZE:
                // Note: In this framework, FREE children need absolute window coordinates.
                // X: shelf absolute x + shelf margin + cumulative width
                // Y: shelf absolute y + fixed bottom 9px above tray lip
                float lip_height = 10.0f;
                float total_h = m_base_size * 2.5f;
                int icon_bottom_y = total_h - lip_height - 9; 

                // Add spacing before each child (except the first one)
                if (count > 0) total_children_width += spacing();

                child->set_position(x() + margin() + total_children_width, y() + icon_bottom_y - child->fixed_size());
                child->set_size(child->fixed_size(), child->fixed_size());
                child->calculate_layout();

                total_children_width += child->fixed_size();
                count++;
            }
        }

        // 2. Update size: content width + margins on both sides
        int content_width = total_children_width + (margin() * 2);
        float total_height = m_base_size * 2.5f;
        set_size(content_width, total_height);
        set_width(content_width);
        set_fixed_size(content_width);

        Widget::calculate_layout();
    }

    void DockShelf::draw(GraphicsContext &gc)
    {
        // Clear the widget area to prevent redrawing artifacts over old frames
        gc.clearRect(x(), y(), width(), height());

        // Draw the 3D OS X Mountain Lion Shelf
        float w = width();
        float h = height(); // 160

        // Shelf geometry measurements
        float lip_height = 10.0f;
        float tray_height = m_base_size * 1.5625f;
        float tray_top_y = h - tray_height;
        float tray_bottom_y = h - lip_height;

        float perspective_offset = 20.0f; // Trapezoid slant width

        // 1. Draw the tray surface (Trapezoid from background to foreground)
        std::vector<PolygonPoint> surface_points = {
            {static_cast<int>(x() + perspective_offset), static_cast<int>(y() + tray_top_y),
             0}, // Top Left
            {static_cast<int>(x() + w - perspective_offset), static_cast<int>(y() + tray_top_y),
             0},                                                                   // Top Right
            {static_cast<int>(x() + w), static_cast<int>(y() + tray_bottom_y), 0}, // Bottom Right
            {static_cast<int>(x()), static_cast<int>(y() + tray_bottom_y), 0}      // Bottom Left
        };

        // Soft white translucent gradient for the glass shelf
        gc.fillLinearGradientPolygon(surface_points, Color(1.0f, 1.0f, 1.0f, 0.2f),
                                     Color(1.0f, 1.0f, 1.0f, 0.6f),
                                     true // vertical
        );

        // 2. Draw the front lip (Edge)
        gc.fillLinearGradientRect(x(), y() + tray_bottom_y, w, lip_height,
                                  Color(0.8f, 0.8f, 0.8f, 0.9f), Color(0.4f, 0.4f, 0.4f, 0.8f),
                                  true, // vertical
                                  CornerRadius(0, 0, 4, 4));

        // 3. Glare/Shine effects
        // Top edge of the lip shine
        gc.setColor(Color(1.0f, 1.0f, 1.0f, 0.8f));
        gc.drawLine(x(), y() + tray_bottom_y + 1, x() + w, y() + tray_bottom_y + 1, 1.0f);

        // Bottom edge of the lip shadow
        gc.setColor(Color(0.1f, 0.1f, 0.1f, 0.5f));
        gc.drawLine(x() + 4, y() + tray_bottom_y + lip_height, x() + w - 4,
                    y() + tray_bottom_y + lip_height, 1.0f);

        // Separating line acting as the back wall intersection
        gc.setColor(Color(1.0f, 1.0f, 1.0f, 0.3f));
        gc.drawLine(x() + perspective_offset, y() + tray_top_y, x() + w - perspective_offset,
                    y() + tray_top_y, 1.0f);

        gc.setColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

} // namespace horizon
