#include "horizon/LayerWindow.hpp"
#include <horizon/LayerApplication.hpp>
#include <horizon/WaylandSurface.hpp>

namespace horizon
{
    LayerWindow::LayerWindow(Application* app, const std::string &namespace_id, uint32_t layer)
        : Window(app, namespace_id, 0, 0, false), m_namespace(namespace_id), m_layer(layer)
    {
        w_surface()->setup_layer_surface(m_layer, m_namespace);
    }

    LayerWindow::~LayerWindow() = default;

    void LayerWindow::set_anchor(uint32_t anchor)
    {
        w_surface()->set_layer_anchor(anchor);
    }

    void LayerWindow::set_exclusive_zone(int32_t zone)
    {
        w_surface()->set_layer_exclusive_zone(zone);
    }

    void LayerWindow::set_keyboard_interactivity(uint32_t interactivity)
    {
        w_surface()->set_layer_keyboard_interactivity(interactivity);
    }

    void LayerWindow::set_layer_size(uint32_t width, uint32_t height)
    {
        w_surface()->set_layer_size(width, height);
    }

    void LayerWindow::commit()
    {
        w_surface()->commit();
    }
 
    void LayerWindow::on_resize(int width, int height)
    {
        Window::on_resize(width, height);
        if (auto* layer_app = dynamic_cast<LayerApplication*>(application()))
        {
            layer_app->update_input_region();
        }
    }
} // namespace horizon
