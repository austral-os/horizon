#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Logger.hpp>
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
    
    auto label = std::make_unique<Label>("Click the button to open a new window");
    container->add_child(std::move(label));
    
    auto btn = std::make_unique<Button<AquaObject>>();
    btn->set_text("Open New Window");
    btn->set_fixed_size(40);
    
    btn->when_mouse_press.connect([&app](EventContext &) {
        LOG_INFO << "Button clicked! Creating new window...";
        WaylandWindow* newWin = app.create_window(400, 300);
        newWin->set_name("Dynamic Window");
        
        auto content = std::make_unique<Window>("New Dynamic Window");
        auto lbl = std::make_unique<Label>("I was created dynamically!");
        lbl->set_margin(20);
        content->add_child(std::move(lbl));
        
        newWin->set_root(std::move(content));
    });
    
    container->add_child(std::move(btn));
    mainWindow->add_child(std::move(container));
    
    app.set_root(std::move(mainWindow));

    // 4. Run the application
    LOG_INFO << "Starting application event loop...";
    app.run();

    return 0;
}
