#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Window.hpp>
#include <memory>

using namespace horizon;

int main(int argc, char *argv[])
{
    // 1. Initialize the application
    Application app("org.horizon.multiwindow", 800, 600);
    app.set_name("Multi-Window Example");
    app.set_icon_name("computer");

    // 2. Create the main window content
    auto mainWindow = std::make_unique<Window>("Main Window");

    auto container = std::make_unique<Widget>();
    container->set_margin(20);
    container->set_spacing(10);

    // We keep a raw pointer to the label to update it later
    auto labelPtr = std::make_unique<Label>("No data received yet");
    Label *statusLabel = labelPtr.get();
    container->add_child(std::move(labelPtr));

    auto btn = std::make_unique<Button<AquaObject>>();
    btn->set_text("Open Input Dialog");
    btn->set_fixed_size(40);

    btn->when_mouse_press.connect(
        [&app, statusLabel](EventContext &)
        {
            LOG_INFO << "Opening input dialog...";
            WaylandWindow *dialog = app.create_dialog(nullptr, 400, 300);
            dialog->set_name("Input Dialog");

            auto content = std::make_unique<Window>("Enter Text");
            content->set_spacing(15);

            auto container = std::make_unique<Widget>();
            container->set_margin(20);
            container->set_spacing(10);

            auto box = std::make_unique<TextBox<TextPolicy>>();
            box->set_placeholder("Type something here...");
            box->set_fixed_size(40);
            TextBox<TextPolicy> *textBoxPtr = box.get();
            container->add_child(std::move(box));

            auto acceptBtn = std::make_unique<Button<AquaObject>>();
            acceptBtn->set_text("Accept");
            acceptBtn->set_fixed_size(40);

            acceptBtn->when_mouse_press.connect(
                [statusLabel, textBoxPtr, dialog](EventContext &)
                {
                    std::string val = textBoxPtr->text();
                    LOG_INFO << "Sending data to main window: " << val;

                    // Update the label in the main window
                    statusLabel->set_text("Received: " + val);

                    // Close this dialog
                    dialog->quit();
                });

            container->add_child(std::move(acceptBtn));
            content->add_child(std::move(container));
            dialog->set_root(std::move(content));
        });

    container->add_child(std::move(btn));
    mainWindow->add_child(std::move(container));

    app.set_root(std::move(mainWindow));

    // 4. Run the application
    LOG_INFO << "Starting application event loop...";
    app.run();

    return 0;
}
