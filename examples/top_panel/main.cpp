#include "horizon/Label.hpp"
#include "horizon/OverlayApplication.hpp"
#include "horizon/Widget.hpp"
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <iostream>
#include <memory>

using namespace horizon;

class MenuWidget : public Widget
{
public:
    MenuWidget() : Widget()
    {
        set_background_color({1.0f, 1.0f, 1.0f, 1.0f}); // Solid white for testing visibility

        auto label = std::make_unique<Label>("Austral");
        label->set_text_color({0.0f, 0.0f, 0.0f, 1.0f}); // Black text on white background
        label->set_margin(20);
        add_child(std::move(label));
    }
};

int main(int argc, char *argv[])
{
    try
    {
        // Create an overlay application
        auto app = std::make_unique<OverlayApplication>("top_panel",
                                                        3); // 3 = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY

        // Anchor to TOP, LEFT, RIGHT to occupy the top of the screen
        app->set_anchor(1 | 4 | 8); // TOP | LEFT | RIGHT

        // Set height to 40, width to 0 (ignored by anchors)
        app->set_size(0, 32);

        // Set exclusive zone to 32 so other windows don't overlap
        app->set_exclusive_zone(32);

        // Enable keyboard interactivity to catch the Escape key
        app->set_keyboard_interactivity(
            1); // 1 = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE

        auto root = std::make_unique<Widget>();
        root->set_background_color({0.0f, 0.0f, 0.0f, 0.5f}); // Slightly darker background

        auto menu = std::make_unique<MenuWidget>();
        // No need to set position/size, it will fill the 40px height root by default if layout is
        // vertical/horizontal

        root->add_child(std::move(menu));
        app->set_root(std::move(root));

        std::cout << "Top Panel started (32px)." << std::endl;

        app->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
