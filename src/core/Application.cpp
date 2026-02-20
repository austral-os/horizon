#include "horizon/GraphicsContext.hpp"
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <iostream>
#include <stdexcept>
#include <wayland-client-core.h>
#include <wayland-client.h>

namespace horizon
{

    static void registry_global(void *data, wl_registry *registry, uint32_t id,
                                const char *interface, uint32_t version)
    {
        Application *app = static_cast<Application *>(data);

        if (strcmp(interface, "wl_compositor") == 0)
        {
            app->set_wl_compositor(static_cast<wl_compositor *>(
                wl_registry_bind(registry, id, &wl_compositor_interface, 4)));
        }
        else if (strcmp(interface, "wl_shm") == 0)
        {
            app->set_wl_shm(
                static_cast<wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, 1)));
        }
    }

    static void registry_global_remove(void *data, wl_registry *registry, uint32_t id)
    {
        // not implemented
    }

    Application::Application()
    {
        // Inicialización del sistema
        init_wayland();
    }

    // Constructor de movimiento
    Application::Application(Application &&other) noexcept
        : m_display(other.m_display), m_registry(other.m_registry),
          m_compositor(other.m_compositor), m_shm(other.m_shm), m_is_running(other.m_is_running),
          m_root(std::move(other.m_root))
    {
        other.m_display = nullptr;
        other.m_registry = nullptr;
        other.m_compositor = nullptr;
        other.m_shm = nullptr;
        other.m_is_running = false;
    }

    // Operador de asignación de movimiento
    Application &Application::operator=(Application &&other) noexcept
    {
        if (this != &other)
        {
            close_wayland();
            m_display = other.m_display;
            m_registry = other.m_registry;
            m_compositor = other.m_compositor;
            m_shm = other.m_shm;
            m_is_running = other.m_is_running;
            m_root = std::move(other.m_root);

            other.m_display = nullptr;
            other.m_registry = nullptr;
            other.m_compositor = nullptr;
            other.m_shm = nullptr;
            other.m_is_running = false;
        }
        return *this;
    }

    Application::~Application()
    {
        // Limpieza
        close_wayland();
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
            GraphicsContext ctx;
            m_root->render(ctx);
        }

        //}
        quit();

        std::cout << "Application finished." << std::endl;
    }

    void Application::quit()
    {
        m_is_running = false;
    }

    void Application::init_wayland()
    {
        m_display = wl_display_connect(nullptr);
        if (!m_display)
        {
            throw std::runtime_error("No se pudo conectar al servidor Wayland.");
        }

        m_registry = wl_display_get_registry(m_display);
        if (!m_registry)
        {
            wl_display_disconnect(m_display);
            m_display = nullptr;
            throw std::runtime_error("No se pudo obtener el registro.");
        }

        // Listener del registry
        static const wl_registry_listener listener = {registry_global, registry_global_remove};
        wl_registry_add_listener(m_registry, &listener, this);

        // Roundtrip inicial para que los globals estén disponibles
        wl_display_roundtrip(m_display);

        std::cout << "Wayland initialized." << std::endl;
    }

    void Application::close_wayland()
    {
        if (m_registry)
        {
            wl_registry_destroy(m_registry);
            m_registry = nullptr;
        }

        if (m_display)
        {
            wl_display_disconnect(m_display);
            m_display = nullptr;
        }
    }

    void Application::dispatch_events()
    {
        if (!m_display)
        {
            return;
        }
        wl_display_dispatch(m_display);
        wl_display_flush(m_display);
    }

    void Application::set_wl_compositor(struct wl_compositor *compositor)
    {
        m_compositor = compositor;
    }

    void Application::set_wl_shm(struct wl_shm *shm)
    {
        m_shm = shm;
    }

} // namespace horizon
