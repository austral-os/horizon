#pragma once

#include "horizon/WaylandWindow.hpp"
#include <string>

namespace horizon
{

    /**
     * @class LayerApplication
     * @brief A specialized Application for surfaces using wlr-layer-shell.
     */
    class WaylandLayerWindow : public WaylandWindow
    {
    public:
        /**
         * @brief Constructs a LayerApplication.
         * @param namespace_id A string identifying the application.
         * @param layer The layer to place the surface in (default is overlay).
         */
        WaylandLayerWindow(const std::string &namespace_id,
                           uint32_t layer = 3, bool defer_init = false, int monitor_index = -1); // 3 = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY

        void initialize() override;
        void on_resize(int width, int height) override;

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
        
        /**
         * @brief Sets the margins for the layer surface.
         * @param top Top margin in pixels.
         * @param right Right margin in pixels.
         * @param bottom Bottom margin in pixels.
         * @param left Left margin in pixels.
         */
        void set_margin(int32_t top, int32_t right, int32_t bottom, int32_t left);

        void set_size(uint32_t width, uint32_t height);
        
        /**
         * @brief Sets both the anchor and size, and commits once to avoid invalid intermediate states.
         */
        void set_anchor_and_size(uint32_t anchor, uint32_t width, uint32_t height);

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

        int monitor_index() const { return m_monitor_index; }

    private:
        std::string m_namespace;
        uint32_t m_layer;
        uint32_t m_interactivity{0}; // 0 = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE
        bool m_visible{false};
        int m_monitor_index{-1};

        void update_screen_position();
    };

} // namespace horizon
