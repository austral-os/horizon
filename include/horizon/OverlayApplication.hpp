#pragma once

#include "horizon/Application.hpp"
#include <string>

namespace horizon
{
    /**
     * @class OverlayApplication
     * @brief A specialized Application for overlay surfaces using wlr-layer-shell.
     */
    class OverlayApplication : public Application
    {
    public:
        /**
         * @brief Constructs an OverlayApplication.
         * @param namespace_id A string identifying the application.
         * @param layer The layer to place the surface in (default is overlay).
         */
        OverlayApplication(const std::string &namespace_id,
                           uint32_t layer = 3); // 3 = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY

        /**
         * @brief Sets the anchor for the overlay.
         * @param anchor Bitmask of zwlr_layer_surface_v1_anchor values.
         */
        void set_anchor(uint32_t anchor);

        /**
         * @brief Sets the exclusive zone for the overlay.
         * @param zone The exclusive zone in pixels.
         */
        void set_exclusive_zone(int32_t zone);

        /**
         * @brief Sets the keyboard interactivity for the overlay.
         * @param interactivity Bitmask of zwlr_layer_surface_v1_keyboard_interactivity values.
         */
        void set_keyboard_interactivity(uint32_t interactivity);
        void set_size(uint32_t width, uint32_t height);

        /**
         * @brief Shows or hides the overlay.
         */
        void set_visible(bool visible);

    private:
        std::string m_namespace;
        uint32_t m_layer;
    };
} // namespace horizon
