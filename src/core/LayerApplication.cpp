#include "horizon/LayerApplication.hpp"
#include <horizon/LayerWindow.hpp>
#include <horizon/WaylandSurface.hpp>
#include <horizon/Window.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
 
namespace horizon
{
    LayerApplication::LayerApplication(const std::string &namespace_id, uint32_t layer)
        : Application(namespace_id, 0, 0, true), m_namespace(namespace_id), m_layer(layer)
    {
        // For LayerApplication, we create a primary window that manages the layer surface
        auto window = std::make_unique<LayerWindow>(this, namespace_id, m_layer);
        m_main_window = window.get();
        
        // Update input region whenever the surface size changes
        add_timer(0, [this]() {
            if (m_main_window) {
                // Initial setup
                w_surface()->commit();
            }
        }, false);

        set_root_window(std::move(window));
    }
 
    WaylandSurface* LayerApplication::w_surface() const
    {
        return m_main_window ? m_main_window->w_surface() : nullptr;
    }

    void LayerApplication::set_anchor(uint32_t anchor)
    {
        if (m_main_window) {
            m_main_window->set_anchor(anchor);
            m_main_window->commit();
        }
    }
 
    void LayerApplication::set_exclusive_zone(int32_t zone)
    {
        if (m_main_window) {
            m_main_window->set_exclusive_zone(zone);
            m_main_window->commit();
        }
    }
 
    void LayerApplication::set_keyboard_interactivity(uint32_t interactivity)
    {
        m_interactivity = interactivity;
        if (m_main_window) {
            m_main_window->set_keyboard_interactivity(interactivity);
            m_main_window->commit();
        }
    }
 
    void LayerApplication::set_size(uint32_t width, uint32_t height)
    {
        if (m_main_window) {
            m_main_window->set_layer_size(width, height);
            m_main_window->commit();
            if (m_visible)
            {
                update_input_region();
            }
        }
    }
 
    void LayerApplication::set_blur(bool blur)
    {
        if (w_surface()) {
            w_surface()->set_blur(blur);
            w_surface()->commit();
        }
    }
 
    void LayerApplication::set_visible(bool visible)
    {
        m_visible = visible;
        if (w_surface()) {
            if (visible)
            {
                // Restore full input region
                update_input_region();
                w_surface()->set_layer_keyboard_interactivity(m_interactivity);
                w_surface()->commit();
            }
            else
            {
                // Clear input region to make it click-through
                w_surface()->clear_input_region();
                w_surface()->set_layer_keyboard_interactivity(ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
                w_surface()->commit();
            }
        }
    }

    int LayerApplication::get_monitor_count() const
    {
        return w_surface() ? w_surface()->monitors().size() : 0;
    }
 
    void LayerApplication::move_to_monitor(int index)
    {
        if (w_surface()) {
            auto *output = w_surface()->get_monitor(index);
            if (output)
            {
                w_surface()->move_layer_to_monitor(output);
            }
        }
    }
 
    void LayerApplication::update_input_region()
    {
        if (w_surface() && m_visible)
        {
            w_surface()->set_input_region(0, 0, w_surface()->width(), w_surface()->height());
            w_surface()->commit();
        }
    }
} // namespace horizon
