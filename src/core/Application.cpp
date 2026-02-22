#include "horizon/CairoGraphicsContext.hpp"
#include <cstdio>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <iostream>
#include <linux/input-event-codes.h>
#include <wayland-client-core.h>
#include <wayland-client.h>

namespace horizon
{

    Application::Application(int w, int h)
    {
        // Inicialización del sistema
        m_surface = std::make_unique<WaylandSurface>(w, h);
        m_surface->init();
        m_surface->set_event_listener(this);
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

    void Application::on_pointer_event(const PointerEvent &event)
    {
        m_pointer_x = event.x;
        m_pointer_y = event.y;

        switch (event.type)
        {
        case PointerEvent::Type::Move:
            handle_move(event);
            break;

        case PointerEvent::Type::Press:
            handle_press(event);
            break;

        case PointerEvent::Type::Release:
            handle_release(event);
            break;

        case PointerEvent::Type::Leave:
            m_pressed = nullptr;
            m_hovered = nullptr;
            break;

        case PointerEvent::Type::Enter:
            handle_move(event);
            break;

        default:
            break;
        }
    }

    void Application::on_resize(int width, int height)
    {
        if (m_root)
        {
            m_root->set_size(width, height);
        }
        m_dirty = true;
    }

    void Application::on_key_event(const KeyEvent &event)
    {
        std::cout << "Key event: " << event.key << std::endl;

        switch (event.type)
        {
        case KeyEvent::Type::Press:
            handle_key_press(event);
            break;

        case KeyEvent::Type::Release:
            handle_key_release(event);
            break;

        default:
            break;
        }
    }

    void Application::handle_key_press(const KeyEvent &event)
    {

        if (event.key == KEY_ESC)
        {
            quit();
            return;
        }

        if (!m_root)
            return;

        m_root->on_key_press(event.key);
    }

    void Application::handle_key_release(const KeyEvent &event)
    {
        if (!m_root)
            return;

        m_root->on_key_release(event.key);
    }

    void Application::handle_move(const PointerEvent &event)
    {
        if (!m_root)
            return;

        Widget *under = m_root->hit_test(event.x, event.y);

        if (under != m_hovered)
        {
            if (m_hovered)
                m_hovered->on_mouse_leave();

            m_hovered = under;

            if (m_hovered)
            {
                m_hovered->on_mouse_enter();
                m_surface->set_cursor(m_hovered->cursor_type());
            }
            else
            {
                m_surface->set_cursor(CursorType::Default);
            }
        }

        if (m_pressed)
        {
            m_pressed->on_mouse_drag(event.x, event.y);
        }
        else if (m_hovered)
        {
            m_hovered->on_mouse_hover(event.x, event.y);
        }
    }

    void Application::handle_press(const PointerEvent &event)
    {
        if (!m_root)
            return;

        Widget *under = m_root->hit_test(event.x, event.y);

        if (under)
        {
            m_last_serial = event.serial;
            m_pressed = under;
            m_pressed->on_mouse_press(event.button);
        }
    }

    void Application::handle_release(const PointerEvent &event)
    {
        if (m_pressed)
        {
            m_pressed->on_mouse_release(event.button);
            m_pressed = nullptr;
        }
    }

    void Application::set_root(std::unique_ptr<Widget> root)
    {
        m_root = std::move(root);
        if (m_root)
        {
            m_root->m_app = this;
        }
    }

    void Application::run()
    {
        m_is_running = true;

        while (m_is_running && wl_display_dispatch(m_surface->display()) != -1)
        {
            if (m_dirty && m_root)
            {
                CairoGraphicContext ctx(m_surface->data(), m_surface->width(), m_surface->height());
                m_root->render(ctx);

                wl_surface_attach(m_surface->surface(), m_surface->buffer(), 0, 0);
                wl_surface_damage(m_surface->surface(), 0, 0, m_surface->width(),
                                  m_surface->height());
                wl_surface_commit(m_surface->surface());

                m_dirty = false;
            }
        }

        quit();

        std::cout << "Application finished." << std::endl;
    }

    void Application::request_move()
    {
        if (m_surface)
        {
            m_surface->request_move(m_last_serial);
        }
    }

    void Application::maximize()
    {
        if (m_surface)
        {
            m_surface->request_maximize();
        }
    }

    void Application::minimize()
    {
        if (m_surface)
        {
            m_surface->request_minimize();
        }
    }

    void Application::restore()
    {
        if (m_surface)
        {
            m_surface->request_restore();
        }
    }

    bool Application::is_maximized() const
    {
        return m_surface && m_surface->is_maximized();
    }

    void Application::quit()
    {
        m_is_running = false;
    }

    void Application::dispatch_events() {}

} // namespace horizon
