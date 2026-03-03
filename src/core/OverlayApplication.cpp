#include "horizon/OverlayApplication.hpp"
#include <horizon/WaylandSurface.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>

namespace horizon
{
    OverlayApplication::OverlayApplication(const std::string &namespace_id, uint32_t layer)
        : Application(0, 0, true), m_namespace(namespace_id), m_layer(layer)
    {
        // Initialization for Layer Shell
        w_surface()->init_display();
        w_surface()->setup_layer_surface(m_layer, m_namespace);
    }

    void OverlayApplication::set_anchor(uint32_t anchor)
    {
        w_surface()->set_layer_anchor(anchor);
        w_surface()->commit();
    }

    void OverlayApplication::set_exclusive_zone(int32_t zone)
    {
        w_surface()->set_layer_exclusive_zone(zone);
        w_surface()->commit();
    }

    void OverlayApplication::set_keyboard_interactivity(uint32_t interactivity)
    {
        w_surface()->set_layer_keyboard_interactivity(interactivity);
        w_surface()->commit();
    }

    void OverlayApplication::set_size(uint32_t width, uint32_t height)
    {
        w_surface()->set_layer_size(width, height);
        w_surface()->commit();
    }

    void OverlayApplication::set_visible(bool visible)
    {
        // For layer shell, hiding usually means unmapping
        // This is a bit more complex, currently we just support showing it
    }
} // namespace horizon
