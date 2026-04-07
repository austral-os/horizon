#include "LaunchpadApplication.hpp"
#include "LaunchpadWindow.hpp"
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>

namespace horizon
{
    LaunchpadApplication::LaunchpadApplication()
        : Application("org.horizon.launchpad", 800, 600, true, true)
    {
        // Create a LayerWindow for the launchpad
        m_window = create_layer_window("org.horizon.launchpad", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY);
        
        m_window->set_name("Launchpad");

        // Full screen anchor
        m_window->set_anchor(ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

        // Request keyboard focus immediately
        m_window->set_keyboard_interactivity(ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);

        // Set exclusive zone to -1 to ignore panels and be truly fullscreen
        m_window->set_exclusive_zone(-1);

        // Enable background blur
        m_window->set_blur(true);

        // Create the launchpad window (widget)
        auto launchpad_widget = std::make_unique<LaunchpadWindow>();

        // Set as root
        m_window->set_root(std::move(launchpad_widget));
        m_window->set_visible(true);
    }

    LaunchpadApplication::~LaunchpadApplication() = default;
} // namespace horizon
