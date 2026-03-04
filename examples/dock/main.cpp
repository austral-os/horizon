#include <cmath>
#include <horizon/Icon.hpp>
#include <horizon/OverlayApplication.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <iostream>
#include <memory>
#include <vector>

using namespace horizon;

/**
 * @brief Custom widget mimicking the Mac OS X Mountain Lion 3D Dock shelf.
 */
class DockShelf : public Widget
{
public:
    DockShelf()
    {
        // Horizontal layout with padding for the shelf appearance
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);
        // Leave space at the bottom for the 3D lip of the shelf
        set_margin(10);
        set_size(0, 100);
    }

    void calculate_layout() override
    {
        Widget::calculate_layout();

        // Let the widget natural width shape around its children
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

        set_size(content_width, height());
        set_fixed_size(content_width); // Prevent horizontal stretching
    }

    void draw(GraphicsContext &gc) override
    {
        // Clear the widget area to prevent redrawing artifacts over old frames
        gc.clearRect(x(), y(), width(), height());

        // Draw the 3D OS X Mountain Lion Shelf
        // 1. Translucent top surface (Trapezoid)
        // 2. Front edge (Rectangle with rounding and gradient)
        // 3. Glare/Shine

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

        // Reset color or state if necessary (none required physically for Horizon context without
        // states)
        gc.setColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }
};

int main(int argc, char *argv[])
{
    try
    {
        // 1. Create the overlay application
        // Anchor to the bottom, left, and right edge to allow size 0
        auto app = std::make_unique<OverlayApplication>("dock", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY);
        app->set_anchor(2 | 4 | 8); // BOTTOM | LEFT | RIGHT
        app->set_size(0, 100);
        app->set_exclusive_zone(100);

        // 2. Root Window
        auto root = std::make_unique<Widget>();
        // Center the shelf horizontally within the available width
        // By setting cross axis alignment to center
        root->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        // Make background completely transparent
        root->set_background_color({0.0f, 0.0f, 0.0f, 0.0f});

        // 3. The Dock Shelf
        auto shelf = std::make_unique<DockShelf>();

        // 4. Populate with icons
        std::vector<std::string> mock_icons = {
            "user-home",  "system-file-manager", "utilities-terminal", "applications-internet",
            "system-run", "preferences-system",  "trash-empty"};

        for (const auto &icon_name : mock_icons)
        {
            auto icn = std::make_unique<Icon>();
            icn->set_icon_name(icon_name);
            icn->set_icon_size(48);

            // Allow icons to be centered vertically somewhat
            icn->set_margin(5);

            shelf->add_child(std::move(icn));
        }

        // Add shelf to root. We use a spacer approach to center the shelf properly
        // because standard layout alignment doesn't strictly center horizontally in horizontal
        // layout yet
        auto left_spacer = std::make_unique<Widget>();
        auto right_spacer = std::make_unique<Widget>();

        root->add_child(std::move(left_spacer));
        root->add_child(std::move(shelf));
        root->add_child(std::move(right_spacer));

        app->set_root(std::move(root));

        std::cout << "Starting Mountain Lion OS X Dock Overlay..." << std::endl;
        app->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
