#include "horizon/CairoGraphicsContext.hpp"
#include <cstdio>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <iostream>
#include <wayland-client-core.h>
#include <wayland-client.h>

namespace horizon
{

    Application::Application(int w, int h)
    {
        // Inicialización del sistema
        m_surface = std::make_unique<WaylandSurface>(w, h);
        m_surface->init();
    }

    // Constructor de movimiento
    Application::Application(Application &&other) noexcept
        : m_is_running(other.m_is_running), m_root(std::move(other.m_root))
    {
        other.m_is_running = false;
    }

    // Operador de asignación de movimiento
    Application &Application::operator=(Application &&other) noexcept
    {
        if (this != &other)
        {
            m_surface->free();

            m_is_running = other.m_is_running;
            m_root = std::move(other.m_root);

            other.m_is_running = false;
        }
        return *this;
    }

    Application::~Application()
    {
        // Limpieza
        m_surface->free();
    }

    void Application::set_root(std::unique_ptr<Widget> root)
    {
        m_root = std::move(root);
    }

    void Application::run()
    {
        m_is_running = true;
        // while (m_is_running)
        //{
        dispatch_events();

        if (m_root)
        {
            CairoGraphicContext ctx(m_surface->data(), m_surface->width(), m_surface->height());
            m_root->render(ctx);
        }

        wl_surface_attach(m_surface->surface(), m_surface->buffer(), 0, 0);
        wl_surface_damage(m_surface->surface(), 0, 0, m_surface->width(), m_surface->height());
        wl_surface_commit(m_surface->surface());

        getchar();

        //}
        quit();

        std::cout << "Application finished." << std::endl;
    }

    void Application::quit()
    {
        m_is_running = false;
    }

    void Application::dispatch_events() {}

} // namespace horizon
