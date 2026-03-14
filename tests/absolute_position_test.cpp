#include <horizon/WaylandWindow.hpp>
#include <horizon/Widget.hpp>
#include <iostream>
#include <cassert>

using namespace horizon;

int main() {
    // Mock WaylandWindow
    WaylandWindow app("test.app", 800, 600, true); // true to defer init
    app.set_screen_position(100, 200);

    // Create root widget
    auto root = std::make_unique<Widget>();
    root->set_position(0, 0);
    Widget* root_ptr = root.get();
    app.set_root(std::move(root));

    // Create child widget
    auto child = std::make_unique<Widget>();
    child->set_position(50, 50);
    Widget* child_ptr = child.get();
    root_ptr->add_child(std::move(child));

    // Create grandchild widget
    auto grandchild = std::make_unique<Widget>();
    grandchild->set_position(10, 10);
    Widget* grandchild_ptr = grandchild.get();
    child_ptr->add_child(std::move(grandchild));

    // Test absolute positions
    widget_position root_pos = root_ptr->get_absolute_position();
    widget_position child_pos = child_ptr->get_absolute_position();
    widget_position grandchild_pos = grandchild_ptr->get_absolute_position();

    std::cout << "Root Pos: " << root_pos.x << ", " << root_pos.y << std::endl;
    std::cout << "Child Pos: " << child_pos.x << ", " << child_pos.y << std::endl;
    std::cout << "Grandchild Pos: " << grandchild_pos.x << ", " << grandchild_pos.y << std::endl;

    assert(root_pos.x == 100 && root_pos.y == 200);
    assert(child_pos.x == 150 && child_pos.y == 250);
    assert(grandchild_pos.x == 160 && grandchild_pos.y == 260);

    std::cout << "Verification successful!" << std::endl;

    return 0;
}
