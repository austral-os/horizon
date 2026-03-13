#include <horizon/Application.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Logger.hpp>
#include <horizon/ToggleGroupButton.hpp>
#include <horizon/Window.hpp>

using horizon::Application;
using horizon::GroupButton;
using horizon::ToggleGroupButton;
using horizon::Widget;
using horizon::Window;

int main()
{
    try
    {
        Application app("org.horizon.group_button_test", 400, 400);
        app.set_name("GroupButton & ToggleGroupButton Test");

        auto wnd = std::make_unique<Window>(&app, "GroupButton Test");
        wnd->set_size(400, 400);

        auto container = std::make_unique<Widget>();
        container->set_margin(50);
        container->set_spacing(20);

        // Standard GroupButton (no toggle)
        LOG_INFO << "Creating standard GroupButton...";
        auto group = std::make_unique<GroupButton>();
        group->add_item("Regular A");
        group->add_item("Regular B");
        group->add_item("Regular C");
        group->set_fixed_size(40);

        group->when_button_clicked.connect(
            [](horizon::GroupButtonClickEvent &ev)
            {
                LOG_INFO << "Regular Button clicked: " << ev.button_index << " - "
                         << ev.button_text;
            });

        // ToggleGroupButton (previous behavior)
        LOG_INFO << "Creating ToggleGroupButton...";
        auto toggleGroup = std::make_unique<ToggleGroupButton>();
        toggleGroup->add_item("Toggle 1");
        toggleGroup->add_item("Toggle 2");
        toggleGroup->add_item("Toggle 3");
        toggleGroup->set_fixed_size(40);

        toggleGroup->when_button_clicked.connect(
            [](horizon::GroupButtonClickEvent &ev)
            {
                LOG_INFO << "Toggle Button clicked: " << ev.button_index << " - " << ev.button_text;
            });

        container->add_child(std::move(group));
        container->add_child(std::move(toggleGroup));
        wnd->add_child(std::move(container));
        app.set_root(std::move(wnd));

        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }
    return 0;
}
