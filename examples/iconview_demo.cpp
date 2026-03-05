#include <horizon/Application.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/IconView.hpp>
#include <horizon/ScrollArea.hpp>
#include <iostream>

using namespace horizon;

int main(int argc, char *argv[])
{
    auto app = std::make_unique<Application>(600, 400);
    app->set_app_id("org.horizon.iconview_demo");

    auto window = std::make_unique<ApplicationWindow>("IconView Demo");
    window->set_size(600, 400);

    auto scroll_area = std::make_unique<ScrollArea>();

    auto icon_view = std::make_unique<IconView>();
    icon_view->set_margin(20);
    icon_view->set_directory("/home/horacio"); // Use home directory for testing

    scroll_area->set_content(std::move(icon_view));

    window->add_child(std::move(scroll_area));

    app->set_root(std::move(window));

    std::cout << "Starting IconView Demo..." << std::endl;
    app->run();
    return 0;
}
