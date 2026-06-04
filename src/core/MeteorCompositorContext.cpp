#include "horizon/MeteorCompositorContext.hpp"
#include "horizon/Logger.hpp"
#include "horizon/WaylandSurface.hpp"
#include "horizon/WaylandWindow.hpp"
#include <unistd.h>

namespace horizon
{
    MeteorCompositorContext::MeteorCompositorContext(WaylandWindow *app) : m_app(app) {}

    void MeteorCompositorContext::request_move(uint32_t serial)
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_move(serial);
        }
    }

    void MeteorCompositorContext::request_resize(uint32_t serial, uint32_t edge)
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_resize(serial, edge);
        }
    }

    void MeteorCompositorContext::maximize()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_maximize();
        }
    }

    void MeteorCompositorContext::minimize()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_minimize();
        }
    }

    void MeteorCompositorContext::restore(const std::string &token)
    {
        LOG_INFO << "[MeteorContext] Restore requested (Token: "
                 << (token.empty() ? "EMPTY" : token) << ")";

        if (!m_app || !m_app->w_surface())
            return;

        auto *surface = m_app->w_surface();

        // Use activation token if provided (Labwc or future Meteor)
        if (!token.empty())
        {
            surface->activate(token);
        }

        if (m_app->is_minimized() || m_app->was_maximized_before_minimize())
        {
            if (m_app->was_maximized_before_minimize())
            {
                LOG_INFO << "[MeteorContext] Window was maximized before minimize, re-maximizing.";
                surface->request_maximize();
            }
            else
            {
                // METEOR SPECIFIC NUDGE:
                // Since Meteor doesn't support xdg_activation_v1 and xdg_shell lacks "unminimize",
                // we "nudge" the compositor by requesting maximization and a dummy move.
                // This usually forces the window to be restored from minimized state.
                LOG_INFO
                    << "[MeteorContext] Window was minimized, nudging via request_maximize and "
                       "request_move.";
                surface->request_maximize();
                surface->request_move(surface->last_serial());
            }
        }
        else if (m_app->is_maximized())
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

    void MeteorCompositorContext::fullscreen()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_fullscreen();
        }
    }

    void MeteorCompositorContext::unfullscreen()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_unfullscreen();
        }
    }

    bool MeteorCompositorContext::is_maximized() const
    {
        return m_app && m_app->w_surface() && m_app->w_surface()->is_maximized();
    }

    bool MeteorCompositorContext::is_fullscreen() const
    {
        return m_app && m_app->w_surface() && m_app->w_surface()->is_fullscreen();
    }

    void MeteorCompositorContext::set_blur(bool enabled)
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->set_blur(enabled);
        }
    }
} // namespace horizon
