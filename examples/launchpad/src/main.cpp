#include "LaunchpadWindow.hpp"
#include <horizon/LayerApplication.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>

using namespace horizon;

int main(int argc, char *argv[])
{
    // Create a LayerApplication for the launchpad
    // Namespace: "horizon.launchpad", Layer: Overlay
    auto app = std::make_unique<LayerApplication>("org.horizon.launchpad",
                                                  ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY);
    app->set_name("Launchpad");

    // Full screen anchor
    app->set_anchor(ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                    ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

    // Request keyboard focus immediately
    app->set_keyboard_interactivity(ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);

    // Set exclusive zone to -1 to ignore panels and be truly fullscreen
    app->set_exclusive_zone(-1);

    // Enable background blur
    app->set_blur(true);

    // Create the launchpad widget
    auto launchpad_widget = std::make_unique<LaunchpadWidget>(app.get());

    // Add to the main window provided by LayerApplication
    app->main_window()->add_child(std::move(launchpad_widget));
    app->set_visible(true);

    app->run();

    return 0;
}
