#include <horizon/Application.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuBar.hpp>
#include <horizon/Window.hpp>

using namespace horizon;

int main(int argc, char **argv)
{
    Application app("org.horizon.menubar_demo", 800, 600);

    app.set_name("Menubar Demo");

    auto window = std::make_unique<Window>("MenuBar Demo");
    window->set_layout_type(WIDGET_LAYOUT_VERTICAL);

    auto menubar = std::make_unique<MenuBar>();
    menubar->set_fixed_size(30);

    // Create some menus
    auto fileMenu = std::make_unique<Menu>();
    fileMenu->set_title("File");
    fileMenu->add_item("New");
    fileMenu->add_item("Open");
    fileMenu->add_separator();
    fileMenu->add_item("Exit");

    auto editMenu = std::make_unique<Menu>();
    editMenu->set_title("Edit");
    editMenu->add_item("Undo");
    editMenu->add_item("Redo");

    auto helpMenu = std::make_unique<Menu>();
    helpMenu->set_title("Window Management and Help"); // Long title for testing
    helpMenu->add_item("About");

    menubar->add_menu(std::move(fileMenu));
    menubar->add_menu(std::move(editMenu));
    menubar->add_menu(std::move(helpMenu));

    window->add_child(std::move(menubar));
    // Centered label to show content
    auto content = std::make_unique<Label>("Click on the menu titles to select them.");
    content->set_alignment(TextAlignment::Center);
    window->add_child(std::move(content));

    app.set_root(std::move(window));

    app.run();

    return 0;
}
