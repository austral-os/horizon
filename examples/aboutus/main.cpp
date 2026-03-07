
#include "AboutWindow.hpp"
#include <horizon/Application.hpp>
#include <memory>

using horizon::AboutWindow;
using horizon::Application;

int main(int argc, char *argv[])
{
    auto app = std::make_unique<Application>(1000, 600);
    app->set_app_id("org.horizon.aboutus");
    app->set_name("About System");
    app->set_icon_name("computer");

    auto window = std::make_unique<AboutWindow>();

    app->set_root(std::move(window));

    app->run();
    return 0;
}
