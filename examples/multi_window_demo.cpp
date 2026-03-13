#include "horizon/EventsManager.hpp"
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/Window.hpp>
#include <iostream>

using namespace horizon;

int main(int argc, char **argv)
{
    Application app("org.horizon.multiwindow_demo", 800, 600);

    // Main Window
    auto main_window = std::make_unique<Window>(&app, "Horizon Main Window", 600, 400);

    auto root_container = std::make_unique<Widget>();
    root_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    root_container->set_margin(20);
    root_container->set_spacing(10);

    auto label = std::make_unique<Label>("This is the main application window.");
    root_container->add_child(std::move(label));

    auto btn_open_dialog = std::make_unique<Button<AquaObject>>();
    btn_open_dialog->set_text("Open Dialog Window");

    Application *app_ptr = &app;
    btn_open_dialog->when_click.connect(
        [app_ptr](EventContext &ev)
        {
            std::cout << "Creating secondary window..." << std::endl;

            auto dialog = std::make_unique<Window>(app_ptr, "Secondary Dialog", 400, 200);

            auto dialog_content = std::make_unique<Widget>();
            dialog_content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            dialog_content->set_margin(15);

            auto dialog_label = std::make_unique<Label>("Hello! I am a second window.");
            dialog_content->add_child(std::move(dialog_label));

            auto btn_close = std::make_unique<Button<AquaObject>>();
            btn_close->set_text("Close Window");
            Window *dialog_ptr = dialog.get();
            btn_close->when_click.connect(
                [app_ptr, dialog_ptr](EventContext &ev)
                {
                    std::cout << "Closing secondary window..." << std::endl;
                    app_ptr->unregister_window(dialog_ptr);
                });
            dialog_content->add_child(std::move(btn_close));

            dialog->add_child(std::move(dialog_content));
            app_ptr->set_root_window(std::move(dialog));
        });

    root_container->add_child(std::move(btn_open_dialog));

    main_window->add_child(std::move(root_container));
    app.set_root_window(std::move(main_window));

    std::cout << "Starting Multi-Window Demo..." << std::endl;
    app.run();

    return 0;
}
