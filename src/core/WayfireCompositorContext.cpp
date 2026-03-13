#include "horizon/WayfireCompositorContext.hpp"
#include "horizon/Application.hpp"
#include "horizon/Application.hpp" // This header might need to be changed to horizon/Window.hpp if Window is a separate class. Assuming it's available.
#include "horizon/Logger.hpp"
#include "horizon/WaylandSurface.hpp"
#include <unistd.h>

namespace horizon
{
    WayfireCompositorContext::WayfireCompositorContext(Window *window) : m_window(window) {}

    void WayfireCompositorContext::request_move(uint32_t serial)
    {
        if (m_window && m_window->w_surface())
        {
            m_window->w_surface()->request_move(serial);
        }
    }

    void WayfireCompositorContext::request_resize(uint32_t serial, uint32_t edge)
    {
        if (m_window && m_window->w_surface())
        {
            m_window->w_surface()->request_resize(serial, edge);
        }
    }

    void WayfireCompositorContext::maximize()
    {
        if (m_window && m_window->w_surface())
        {
            m_window->w_surface()->request_maximize();
        }
    }

    void WayfireCompositorContext::minimize()
    {
        if (m_window && m_window->w_surface())
        {
            m_window->w_surface()->request_minimize();
        }
    }

    void WayfireCompositorContext::restore(const std::string &token)
    {
        LOG_INFO << "[WayfireContext] Restore requested (Token: "
                 << (token.empty() ? "EMPTY" : token) << ")";

        if (!m_window || !m_window->w_surface())
            return;

        auto *surface = m_window->w_surface();

        // Use activation token if provided (Labwc or future Wayfire)
        if (!token.empty())
        {
            surface->activate(token);
        }

        if (m_window->is_minimized() || m_window->was_maximized_before_minimize())
        {
            if (m_window->was_maximized_before_minimize())
            {
                LOG_INFO << "[WayfireContext] Window was maximized before minimize, re-maximizing.";
                surface->request_maximize();
            }
            else
            {
                // WAYFIRE SPECIFIC NUDGE:
                // Since Wayfire doesn't support xdg_activation_v1 and xdg_shell lacks "unminimize",
                // we "nudge" the compositor by requesting maximization and a dummy move.
                // This usually forces the window to be restored from minimized state.
                LOG_INFO
                    << "[WayfireContext] Window was minimized, nudging via request_maximize and "
                       "request_move.";
                surface->request_maximize();
                surface->request_move(surface->last_serial());
            }
        }
        else if (m_window->is_maximized())
        {
            surface->request_restore();
        }
        else
        {
            // If it's already visible and not maximized, just activate it (if token allowed)
            // or maximize if we want to ensure it comes to front.
            surface->request_maximize();
        }

        surface->commit();
    }

    void WayfireCompositorContext::fullscreen()
    {
        if (m_window && m_window->w_surface())
        {
            m_window->w_surface()->request_fullscreen();
        }
    }

    void WayfireCompositorContext::unfullscreen()
    {
        if (m_window && m_window->w_surface())
        {
            m_window->w_surface()->request_unfullscreen();
        }
    }

    bool WayfireCompositorContext::is_maximized() const
    {
        return m_window && m_window->w_surface() && m_window->w_surface()->is_maximized();
    }

    bool WayfireCompositorContext::is_fullscreen() const
    {
        return m_window && m_window->w_surface() && m_window->w_surface()->is_fullscreen();
    }

    void WayfireCompositorContext::set_blur(bool enabled)
    {
        if (m_window && m_window->w_surface())
        {
            m_window->w_surface()->set_blur(enabled);
        }
    }
} // namespace horizon
