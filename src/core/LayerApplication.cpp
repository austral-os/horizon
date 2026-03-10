#include "horizon/LayerApplication.hpp"
#include <horizon/WaylandSurface.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>

namespace horizon
{
    LayerApplication::LayerApplication(const std::string &namespace_id, uint32_t layer)
        : Application(namespace_id, 0, 0, true), m_namespace(namespace_id), m_layer(layer),
          m_interactivity(0)
    {
        // Initialization for Layer Shell
        w_surface()->init_display();
        w_surface()->setup_layer_surface(m_layer, m_namespace);

        // Update input region whenever the surface size changes
        add_on_resize(
            [this](int w, int h)
            {
                if (m_visible)
                {
                    w_surface()->set_input_region(0, 0, w, h);
                }
            });
    }

    void LayerApplication::set_anchor(uint32_t anchor)
    {
        w_surface()->set_layer_anchor(anchor);
        w_surface()->commit();
    }

    void LayerApplication::set_exclusive_zone(int32_t zone)
    {
        w_surface()->set_layer_exclusive_zone(zone);
        w_surface()->commit();
    }

    void LayerApplication::set_keyboard_interactivity(uint32_t interactivity)
    {
        m_interactivity = interactivity;
        w_surface()->set_layer_keyboard_interactivity(interactivity);
        w_surface()->commit();
    }

    void LayerApplication::set_size(uint32_t width, uint32_t height)
    {
        w_surface()->set_layer_size(width, height);
        w_surface()->commit();
    }

    void LayerApplication::set_visible(bool visible)
    {
        m_visible = visible;
        if (visible)
        {
            // Restore full input region
            w_surface()->set_input_region(0, 0, w_surface()->width(), w_surface()->height());
            w_surface()->set_layer_keyboard_interactivity(m_interactivity);
            w_surface()->commit();
        }
        else
        {
            // Clear input region to make it click-through
            w_surface()->clear_input_region();
            set_keyboard_interactivity(ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
        }
    }
    int LayerApplication::get_monitor_count() const
    {
        return w_surface()->monitors().size();
    }

    void LayerApplication::move_to_monitor(int index)
    {
        auto *output = w_surface()->get_monitor(index);
        if (output)
        {
            w_surface()->move_layer_to_monitor(output);
        }
    }
} // namespace horizon
