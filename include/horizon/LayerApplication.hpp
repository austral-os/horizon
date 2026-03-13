#pragma once
 
#include "horizon/Application.hpp"
#include <string>
 
namespace horizon
{
    class Window;
    class WaylandSurface;

    /**
     * @class LayerApplication
     * @brief A specialized Application for surfaces using wlr-layer-shell.
     */
    class LayerApplication : public Application
    {
    public:
        /**
         * @brief Constructs a LayerApplication.
         * @param namespace_id A string identifying the application.
         * @param layer The layer to place the surface in (default is overlay).
         */
        LayerApplication(const std::string &namespace_id,
                         uint32_t layer = 3); // 3 = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY
 
        /**
         * @brief Sets the anchor for the layer surface.
         * @param anchor Bitmask of zwlr_layer_surface_v1_anchor values.
         */
        void set_anchor(uint32_t anchor);
 
        /**
         * @brief Sets the exclusive zone for the layer surface.
         * @param zone The exclusive zone in pixels.
         */
        void set_exclusive_zone(int32_t zone);
 
        /**
         * @brief Sets the keyboard interactivity for the layer surface.
         * @param interactivity Bitmask of zwlr_layer_surface_v1_keyboard_interactivity values.
         */
        void set_keyboard_interactivity(uint32_t interactivity);
        void set_size(uint32_t width, uint32_t height);
        void set_blur(bool blur);
 
        int get_monitor_count() const;
        void move_to_monitor(int index);
 
        /**
         * @brief Shows or hides the layer surface.
         */
        void set_visible(bool visible);
 
        bool is_transparent_surface() const override
        {
            return true;
        }

        Window* main_window() const { return m_main_window; }
        WaylandSurface* w_surface() const;
 
    private:
        Window* m_main_window{nullptr};
        std::string m_namespace;
        uint32_t m_layer;
        uint32_t m_interactivity{0}; // 0 = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE
        bool m_visible{false};
    };
 
    // For backward compatibility
    using OverlayApplication = LayerApplication;
} // namespace horizon
