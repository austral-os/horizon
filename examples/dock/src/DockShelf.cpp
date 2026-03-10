#include "DockShelf.hpp"
#include <horizon/GraphicsContext.hpp>
#include <vector>

namespace horizon
{

    DockShelf::DockShelf()
    {
        // Horizontal layout with padding for the shelf appearance
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);
        // Leave space at the bottom for the 3D lip of the shelf
        set_margin(20);
        set_size(0, 100);
    }

    void DockShelf::calculate_layout()
    {
        // 1. Calculate the required content width based on children
        int content_width = 0;
        for (const auto &child : children())
        {
            if (child->is_visible() && child->fixed_size() > 0)
            {
                content_width += child->fixed_size() + spacing();
            }
        }
        if (!children().empty() && content_width > 0)
        {
            content_width -= spacing();
        }

        content_width += 40; // Simulated Left + Right padding

        // 2. Update size BEFORE base calculate_layout so parents and centering use new dimensions
        set_size(content_width, height());
        set_fixed_size(content_width);

        Widget::calculate_layout();
    }

    void DockShelf::draw(GraphicsContext &gc)
    {
        // Clear the widget area to prevent redrawing artifacts over old frames
        gc.clearRect(x(), y(), width(), height());

        // Draw the 3D OS X Mountain Lion Shelf
        float w = width();
        float h = height();

        // Shelf geometry measurements
        float lip_height = 10.0f;
        float tray_top_y = 15.0f; // Tray starts 15px down from the top bounds
        float tray_bottom_y = h - lip_height - 2.0f;

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
