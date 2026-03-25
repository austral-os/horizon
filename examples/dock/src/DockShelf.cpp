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
                    // ctx.x is window-relative, but we need shelf-local for calculate_layout
                    widget_position pos = get_absolute_position();
                    int window_x = pos.x - (application() ? application()->screen_x() : 0);
                    m_mouse_x = ctx.x - window_x;
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
        const int base_size = 64;
        const int max_extra = 64;
        const float radius = 120.0f; // Distance of influence pixels

        // 1. Calculate the required content width based on children (with magnification)
        int total_children_width = 0;
        int count = 0;

        for (const auto &child : children())
        {
            if (child->is_visible())
            {
                Icon *icon_child = dynamic_cast<Icon *>(child.get());
                int current_icon_size = base_size;

                if (m_magnification_enabled && m_mouse_over)
                {
                    // Calculate center relative to drawing area start
                    float child_center_x = (float)total_children_width + child->fixed_size() / 2.0f;
                    // Mouse x is shelf-local, content starts at margin()
                    float dist = std::abs((child_center_x + margin()) - m_mouse_x);
                    float scale = std::exp(-(dist * dist) / (2 * radius * radius));

                    current_icon_size = base_size + (int)(max_extra * scale);
                    child->set_fixed_size(current_icon_size + child->margin() * 2);
                }
                else
                {
                    child->set_fixed_size(base_size + child->margin() * 2);
                }

                if (icon_child)
                {
                    icon_child->set_icon_size(current_icon_size);
                }

                // MANUALLY SET ABSOLUTE POSITION AND SIZE:
                // Note: In this framework, FREE children need absolute window coordinates.
                // X: shelf absolute x + shelf margin + cumulative width
                // Y: shelf absolute y + fixed bottom 9px above tray lip (y=150)
                int icon_bottom_y = 150 - 9; 
                child->set_position(x() + margin() + total_children_width, y() + icon_bottom_y - child->fixed_size());
                child->set_size(child->fixed_size(), child->fixed_size());
                child->calculate_layout();

                total_children_width += child->fixed_size();
                if (count > 0) total_children_width += spacing();
                count++;
            }
        }

        // 2. Update size: content width + margins on both sides
        int content_width = total_children_width + (margin() * 2);
        set_size(content_width, 160); // Full height for magnification room
        set_width(content_width);


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
        float tray_height = 100.0f;
        float lip_height = 10.0f;
        float tray_top_y = h - tray_height; // Tray starts at y=60
        float tray_bottom_y = h - lip_height; // End surface at 150

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
