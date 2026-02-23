#include "horizon/CairoGraphicsContext.hpp"
#include <cstdio>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/xdg-shell-client-protocol.h>
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

        m_pointer_x = event.x;
        m_pointer_y = event.y;

        // Detectar borde para redimensionado
        uint32_t edge = XDG_TOPLEVEL_RESIZE_EDGE_NONE;
        if (!is_maximized())
        {
            bool top = event.y < m_resize_proximity;
            bool bottom = event.y > m_surface->height() - m_resize_proximity;
            bool left = event.x < m_resize_proximity;
            bool right = event.x > m_surface->width() - m_resize_proximity;

            if (top && left)
                edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT;
            else if (top && right)
                edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT;
            else if (bottom && left)
                edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT;
            else if (bottom && right)
                edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT;
            else if (top)
                edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP;
            else if (bottom)
                edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
            else if (left)
                edge = XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
            else if (right)
                edge = XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
        }

        m_resize_edge = edge;

        if (m_resize_edge != XDG_TOPLEVEL_RESIZE_EDGE_NONE)
        {
            if (m_hovered)
            {
                m_hovered->on_mouse_leave();
                m_hovered = nullptr;
            }

            CursorType cursor = CursorType::Default;
            if (m_resize_edge == XDG_TOPLEVEL_RESIZE_EDGE_TOP ||
                m_resize_edge == XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM)
                cursor = CursorType::ResizeNS;
            else if (m_resize_edge == XDG_TOPLEVEL_RESIZE_EDGE_LEFT ||
                     m_resize_edge == XDG_TOPLEVEL_RESIZE_EDGE_RIGHT)
                cursor = CursorType::ResizeEW;
            else if (m_resize_edge == XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT ||
                     m_resize_edge == XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT)
                cursor = CursorType::ResizeNESW;
            else if (m_resize_edge == XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT ||
                     m_resize_edge == XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT)
                cursor = CursorType::ResizeNWSE;

            m_surface->set_cursor(cursor);
        }
        else
        {
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

        m_last_serial = event.serial;

        if (m_resize_edge != XDG_TOPLEVEL_RESIZE_EDGE_NONE)
        {
            m_surface->request_resize(event.serial, m_resize_edge);
            return;
        }

        Widget *under = m_root->hit_test(event.x, event.y);

        if (under)
        {
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
