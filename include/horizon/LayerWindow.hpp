#pragma once

#include "Window.hpp"
#include <string>

namespace horizon
{
    /**
     * @class LayerWindow
     * @brief A specialized Window that uses wlr-layer-shell.
     */
    class LayerWindow : public Window
    {
    public:
        LayerWindow(Application* app, const std::string &namespace_id, uint32_t layer = 3); // 3 = Overlay
        virtual ~LayerWindow();

        void set_anchor(uint32_t anchor);
        void set_exclusive_zone(int32_t zone);
        void set_keyboard_interactivity(uint32_t interactivity);
        void set_layer_size(uint32_t width, uint32_t height);
        void set_margin(int top, int right, int bottom, int left);
        
        void commit();
 
    protected:
        void on_resize(int width, int height) override;
 
    private:
        std::string m_namespace;
        uint32_t m_layer;
    };
} // namespace horizon
