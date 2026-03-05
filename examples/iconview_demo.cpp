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

    auto icon_view = std::make_unique<IconView>();

    icon_view->set_directories_first(true);
    icon_view->set_zoom(1.0f);
    icon_view->set_directory("/home/horacio"); // Use home directory for testing

    window->add_child(std::move(icon_view));

    app->set_root(std::move(window));

    std::cout << "Starting IconView Demo..." << std::endl;
    app->run();
    return 0;
}
