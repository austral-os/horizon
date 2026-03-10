#include "horizon/WayfireCompositorContext.hpp"
#include "horizon/Application.hpp"
#include "horizon/WaylandSurface.hpp"
#include <unistd.h>

namespace horizon
{
    WayfireCompositorContext::WayfireCompositorContext(Application *app) : m_app(app) {}

    void WayfireCompositorContext::request_move(uint32_t serial)
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_move(serial);
        }
    }

    void WayfireCompositorContext::request_resize(uint32_t serial, uint32_t edge)
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_resize(serial, edge);
        }
    }

    void WayfireCompositorContext::maximize()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_maximize();
        }
    }

    void WayfireCompositorContext::minimize()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_minimize();
        }
    }

    void WayfireCompositorContext::restore(const std::string &token)
    {
        if (!m_app || !m_app->w_surface())
            return;

        auto *surface = m_app->w_surface();

        // Use activation token if provided
        if (!token.empty())
        {
            surface->activate(token);
        }

        if (m_app->is_minimized() || m_app->was_maximized_before_minimize())
        {
            if (m_app->was_maximized_before_minimize())
            {
                surface->request_maximize();
            }
            else
            {
                surface->request_restore();
            }
        }
        else if (m_app->is_maximized())
        {
            surface->request_restore();
        }
        else
        {
            surface->request_maximize();
        }

        surface->commit();
    }

    void WayfireCompositorContext::fullscreen()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_fullscreen();
        }
    }

    void WayfireCompositorContext::unfullscreen()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_unfullscreen();
        }
    }

    bool WayfireCompositorContext::is_maximized() const
    {
        return m_app && m_app->is_maximized();
    }

    bool WayfireCompositorContext::is_fullscreen() const
    {
        return m_app && m_app->is_fullscreen();
    }
} // namespace horizon
