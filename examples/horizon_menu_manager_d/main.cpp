#include <horizon/Label.hpp>
#include <horizon/OverlayApplication.hpp>
#include <horizon/Widget.hpp>
#include <iostream>
#include <memory>

using namespace horizon;

class MenuWidget : public Widget
{
public:
    MenuWidget() : Widget()
    {
        set_background_color({0.0f, 0.0f, 0.0f, 0.0f}); // Fully transparent

        auto label = std::make_unique<Label>("System Menu");
        label->set_text_color({1.0f, 1.0f, 1.0f, 1.0f});
        label->set_margin(15);
        add_child(std::move(label));
    }
};

int main(int argc, char *argv[])
{
    try
    {
        // Create the Menu Manager Daemon
        auto app =
            std::make_unique<OverlayApplication>("horizon_menu_manager_d", 3); // 3 = OVERLAY layer

        // Fullscreen anchor
        app->set_anchor(1 | 2 | 4 | 8); // TOP | BOTTOM | LEFT | RIGHT

        // No exclusive zone - we don't want to move other windows
        app->set_exclusive_zone(0);

        // Enable keyboard interactivity to catch Escape
        app->set_keyboard_interactivity(1); // 1 = EXCLUSIVE

        auto root = std::make_unique<Widget>();
        root->set_background_color({0.0f, 0.0f, 0.0f, 0.0f}); // COMPLETELY TRANSPARENT ROOT

        // Add a mock menu at a specific position
        auto menu = std::make_unique<MenuWidget>();
        menu->set_position(100, 100);
        menu->set_size(250, 200);
        menu->set_position_type(FREE);

        root->add_child(std::move(menu));
        app->set_root(std::move(root));

        std::cout << "Horizon Menu Manager Daemon started (Fullscreen Overlay)." << std::endl;
        std::cout << "Root is transparent. Press Escape to quit." << std::endl;

        app->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
