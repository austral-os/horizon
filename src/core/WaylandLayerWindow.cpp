#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/WaylandSurface.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>

namespace horizon
{
    WaylandLayerWindow::WaylandLayerWindow(const std::string &namespace_id, uint32_t layer, bool defer_init)
        : WaylandWindow(namespace_id, 0, 0, true, false), m_namespace(namespace_id), m_layer(layer),
          m_interactivity(0)
    {
        if (!defer_init)
        {
            initialize();
        }

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

    void WaylandLayerWindow::on_resize(int w, int h)
    {
        WaylandWindow::on_resize(w, h);
        update_screen_position();
    }

    void WaylandLayerWindow::initialize()
    {
        w_surface()->init_display();
        w_surface()->setup_layer_surface(m_layer, m_namespace);
    }

    void WaylandLayerWindow::set_anchor(uint32_t anchor)
    {
        w_surface()->set_layer_anchor(anchor);
        w_surface()->commit();
    }

    void WaylandLayerWindow::set_exclusive_zone(int32_t zone)
    {
        w_surface()->set_layer_exclusive_zone(zone);
        w_surface()->commit();
    }

    void WaylandLayerWindow::set_keyboard_interactivity(uint32_t interactivity)
    {
        m_interactivity = interactivity;
        w_surface()->set_layer_keyboard_interactivity(interactivity);
        w_surface()->commit();
    }

    void WaylandLayerWindow::set_size(uint32_t width, uint32_t height)
    {
        w_surface()->set_layer_size(width, height);
        w_surface()->commit();
    }

    void WaylandLayerWindow::set_visible(bool visible)
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
    int WaylandLayerWindow::get_monitor_count() const
    {
        return w_surface()->monitors().size();
    }

    void WaylandLayerWindow::move_to_monitor(int index)
    {
        auto *output = w_surface()->get_monitor(index);
        if (output)
        {
            w_surface()->move_layer_to_monitor(output);
            update_screen_position();
        }
    }

    void WaylandLayerWindow::update_screen_position()
    {
        int mw = w_surface()->monitor_width();
        int mh = w_surface()->monitor_height();
        int ww = width();
        int wh = height();
        uint32_t anchor = w_surface()->anchor();

        int x = 0;
        int y = 0;

        // X calculation
        if ((anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) && !(anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT))
        {
            x = 0;
        }
        else if ((anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) && !(anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT))
        {
            x = mw - ww;
        }
        else if ((anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) && (anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT))
        {
            x = 0;
        }
        else
        {
            x = (mw - ww) / 2;
        }

        // Y calculation
        if ((anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) && !(anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM))
        {
            y = 0;
        }
        else if ((anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) && !(anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP))
        {
            y = mh - wh;
        }
        else if ((anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) && (anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM))
        {
            y = 0;
        }
        else
        {
            y = (mh - wh) / 2;
        }

        set_screen_position(x, y);
    }
} // namespace horizon
