#include "horizon/LabwcCompositorContext.hpp"
#include "horizon/Application.hpp"
#include "horizon/Logger.hpp"
#include "horizon/WaylandSurface.hpp"
#include <unistd.h>

namespace horizon
{
    LabwcCompositorContext::LabwcCompositorContext(Application *app) : m_app(app) {}

    void LabwcCompositorContext::request_move(uint32_t serial)
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_move(serial);
        }
    }

    void LabwcCompositorContext::request_resize(uint32_t serial, uint32_t edge)
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_resize(serial, edge);
        }
    }

    void LabwcCompositorContext::maximize()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_maximize();
            // We'll let Application handle internal state updates like m_is_minimized
            // but the protocol request happens here.
        }
    }

    void LabwcCompositorContext::minimize()
    {
        if (m_app && m_app->w_surface())
        {
            LOG_INFO << "[LabwcContext] Minimizing window...";
            m_app->w_surface()->request_minimize();
        }
    }

    void LabwcCompositorContext::restore(const std::string &token)
    {
        if (!m_app || !m_app->w_surface())
            return;

        auto *surface = m_app->w_surface();

        // Protocol sequence:
        // 1. If we have an activation token, use it FIRST.
        if (!token.empty())
        {
            surface->activate(token);
        }

        // 2. Request the compositor to un-minimize/maximize
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

        // 3. Force a commit to ensure the requests are sent to the compositor
        surface->commit();
    }

    void LabwcCompositorContext::fullscreen()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_fullscreen();
        }
    }

    void LabwcCompositorContext::unfullscreen()
    {
        if (m_app && m_app->w_surface())
        {
            m_app->w_surface()->request_unfullscreen();
        }
    }

    bool LabwcCompositorContext::is_maximized() const
    {
        return m_app && m_app->is_maximized();
    }

    bool LabwcCompositorContext::is_fullscreen() const
    {
        return m_app && m_app->is_fullscreen();
    }
} // namespace horizon
