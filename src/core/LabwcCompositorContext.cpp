#include "horizon/LabwcCompositorContext.hpp"
#include "horizon/Application.hpp"
#include "horizon/WaylandSurface.hpp"
#include <iostream>

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
            std::cout << "[LabwcContext] Minimizing window..." << std::endl;
            m_app->w_surface()->request_minimize();
        }
    }

    void LabwcCompositorContext::restore(const std::string &token)
    {
        if (!m_app || !m_app->w_surface())
            return;

        auto *surface = m_app->w_surface();

        if (!token.empty())
        {
            surface->activate(token);
        }

        // Exact mechanism as currently used in Application.cpp
        if (m_app->is_minimized())
        {
            // If we were minimized, we just want to bring the window back.
            // If it was maximized before minimizing, keep it maximized.
            if (m_app->is_maximized())
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
            // If it was already visible and maximized, restore to floating
            surface->request_restore();
        }
        else
        {
            // Not minimized and not maximized? This shouldn't happen from the maximize button,
            // but for safety we just maximize.
            surface->request_maximize();
        }

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
