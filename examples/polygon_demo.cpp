#include <cmath>
#include <horizon/Application.hpp>
#include <horizon/AquaPolygon.hpp>
#include <horizon/Menu.hpp>
#include <horizon/Window.hpp>
#include <vector>

using namespace horizon;

int main(int argc, char **argv)
{
    Application app(800, 600);
    app.set_app_id("horizon.polygon_demo");
    app.set_name("Polygon Demo");
    app.set_icon_name("applications-graphics");
    app.set_show_in_dock(true);

    auto window = std::make_unique<Window>("AquaPolygon Demo");
    window->set_size(800, 600);

    // 1. Triangle
    auto triangle = std::make_unique<AquaPolygon>();
    triangle->set_position(50, 50);
    triangle->set_size(150, 150);
    triangle->set_points({
        {75, 10, 20},   // Top
        {140, 140, 20}, // Bottom Right
        {10, 140, 20}   // Bottom Left
    });
    triangle->set_accent_color(WidgetAccentColor::Primary);
    triangle->set_has_border(true);
    window->add_child(std::move(triangle));

    // 2. Hexagon
    auto hexagon = std::make_unique<AquaPolygon>();
    hexagon->set_position(250, 50);
    hexagon->set_size(150, 150);
    std::vector<PolygonPoint> hex_points;
    for (int i = 0; i < 6; ++i)
    {
        double angle = i * M_PI / 3.0;
        hex_points.push_back({static_cast<int>(75 + 60 * std::cos(angle)),
                              static_cast<int>(75 + 60 * std::sin(angle)), 10});
    }
    hexagon->set_points(hex_points);
    hexagon->set_accent_color(WidgetAccentColor::Success);
    hexagon->set_has_border(true);
    window->add_child(std::move(hexagon));

    // 3. Circle from Square (50% radius)
    auto circle = std::make_unique<AquaPolygon>();
    circle->set_position(450, 50);
    circle->set_size(150, 150);
    circle->set_points(
        {{10, 10, 65}, // Each corner has radius = 65 (approx 50% of 130 usable width)
         {140, 10, 65},
         {140, 140, 65},
         {10, 140, 65}});
    circle->set_accent_color(WidgetAccentColor::Error);
    circle->set_has_border(true);
    circle->set_border_size(3.0f);
    window->add_child(std::move(circle));

    // 4. Star ish shape
    auto star = std::make_unique<AquaPolygon>();
    star->set_position(50, 250);
    star->set_size(150, 150);
    star->set_points({{75, 10, 5},
                      {90, 60, 5},
                      {140, 60, 5},
                      {100, 90, 5},
                      {115, 140, 5},
                      {75, 110, 5},
                      {35, 140, 5},
                      {50, 90, 5},
                      {10, 60, 5},
                      {60, 60, 5}});
    star->set_accent_color(WidgetAccentColor::Warning);
    star->set_has_border(false);
    window->add_child(std::move(star));

    app.set_root(std::move(window));

    auto app_menu = new horizon::Menu();
    app_menu->set_title("Polygonal Demo");
    app_menu->set_bold(true);
    app_menu->add_item("Preferences", "Ctrl+,");
    app_menu->add_separator();
    app_menu->add_item("Quit", "Ctrl+Q");

    auto polygon_menu = new horizon::Menu();
    polygon_menu->set_title("Polygon");
    polygon_menu->add_item("Add Triangle", "Ctrl+T");
    polygon_menu->add_item("Add Hexagon", "Ctrl+H");
    polygon_menu->add_separator();
    polygon_menu->add_item("Clear All", "Ctrl+Shift+C");

    auto view_menu = new horizon::Menu();
    view_menu->set_title("View");
    view_menu->add_item("Zoom In", "Ctrl++");
    view_menu->add_item("Zoom Out", "Ctrl+-");

    app.set_global_menu({app_menu, polygon_menu, view_menu});

    app.run();
    return 0;
}
