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

    // Create the launchpad window
    auto launchpad_window = std::make_unique<LaunchpadWindow>();

    // Set as root and run
    app->set_root(std::move(launchpad_window));
    app->set_visible(true);

    app->run();

    return 0;
}
