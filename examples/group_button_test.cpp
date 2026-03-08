#include <horizon/Application.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Window.hpp>
#include <iostream>

using horizon::Application;
using horizon::GroupButton;
using horizon::Widget;
using horizon::Window;

int main()
{
    try
    {
        Application app(400, 300);
        app.set_app_id("horizon.group_button_test");
        app.set_name("GroupButton Test");

        auto wnd = std::make_unique<Window>("GroupButton Test");
        wnd->set_size(400, 300);

        auto container = std::make_unique<Widget>();
        container->set_margin(50);
        container->set_spacing(20);

        auto group = std::make_unique<GroupButton>();
        group->add_item("Option A");
        group->add_item("Option B");
        group->add_item("Option C");
        group->set_fixed_size(40);

        group->when_button_clicked.connect(
            [](horizon::GroupButtonClickEvent &ev)
            {
                std::cout << "Button clicked: " << ev.button_index << " - " << ev.button_text
                          << std::endl;
            });

        container->add_child(std::move(group));
        wnd->add_child(std::move(container));
        app.set_root(std::move(wnd));

        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
