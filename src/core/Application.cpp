#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/Widget.hpp"
#include <cstdio>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <iostream>
#include <linux/input-event-codes.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

namespace horizon
{

    Application::Application(int w, int h)
    {
        // Inicialización del sistema
        m_surface = std::make_unique<WaylandSurface>(w, h);
        m_surface->init();
        m_surface->set_event_listener(this);

        theme_manager = std::make_unique<ThemeManager>();

        theme_manager->when_change.connect(
            [this](EventContext &p)
            {
                std::cout << "Theme changed" << std::endl;
                m_dirty = true;
            });
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

        for (auto const &[id, handler] : m_on_resize_handlers)
        {
            if (handler)
                handler(width, height);
        }
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

        EventContext new_ev = {.sender = nullptr,
                               .type = EventType::KeyPress,
                               .button = event.key,
                               .stop_propagation = false,
                               .data = nullptr,
                               .key = event.key};
        m_root->when_key_press.run(new_ev);
    }

    void Application::handle_key_release(const KeyEvent &event)
    {
        if (!m_root)
            return;

        EventContext new_ev = {.sender = nullptr,
                               .type = EventType::KeyRelease,
                               .button = event.key,
                               .stop_propagation = false,
                               .data = nullptr,
                               .key = event.key};
        m_root->when_key_release.run(new_ev);
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
                EventContext new_ev = {.sender = m_hovered,
                                       .type = EventType::MouseLeave,
                                       .button = event.button,
                                       .stop_propagation = false,
                                       .data = nullptr};
                m_hovered->when_mouse_leave.run(new_ev);
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
                {
                    EventContext new_ev = {.sender = m_hovered,
                                           .type = EventType::MouseLeave,
                                           .button = event.button,
                                           .stop_propagation = false,
                                           .data = nullptr};
                    m_hovered->when_mouse_leave.run(new_ev);
                }

                m_hovered = under;

                if (m_hovered)
                {
                    EventContext new_ev = {.sender = m_hovered,
                                           .type = EventType::MouseEnter,
                                           .button = event.button,
                                           .stop_propagation = false,
                                           .data = nullptr};
                    m_hovered->when_mouse_enter.run(new_ev);
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
            EventContext new_ev = {.sender = m_pressed,
                                   .type = EventType::MouseDrag,
                                   .button = event.button,
                                   .stop_propagation = false,
                                   .data = nullptr};
            m_pressed->when_mouse_drag.run(new_ev);
        }
        else if (m_hovered)
        {
            EventContext new_ev = {.sender = m_hovered,
                                   .type = EventType::MouseHover,
                                   .button = event.button,
                                   .stop_propagation = false,
                                   .data = nullptr};
            m_hovered->when_mouse_hover.run(new_ev);
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

            EventContext new_ev = {.sender = m_pressed,
                                   .type = EventType::MousePress,
                                   .button = event.button,
                                   .stop_propagation = false,
                                   .data = nullptr};

            m_pressed->when_mouse_press.run(new_ev);
            // m_pressed->on_mouse_press(event.button);
        }
    }

    void Application::handle_release(const PointerEvent &event)
    {
        if (m_pressed)
        {
            EventContext new_ev = {.sender = m_pressed,
                                   .type = EventType::MouseRelease,
                                   .button = event.button,
                                   .stop_propagation = false,
                                   .data = nullptr};

            m_pressed->when_mouse_release.run(new_ev);
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

        for (auto const &[id, handler] : m_on_start_handlers)
        {
            if (handler)
                handler();
        }

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
            for (auto const &[id, handler] : m_on_maximize_handlers)
            {
                if (handler)
                    handler(true);
            }
        }
    }

    void Application::minimize()
    {
        if (m_surface)
        {
            m_surface->request_minimize();
            for (auto const &[id, handler] : m_on_minimize_handlers)
            {
                if (handler)
                    handler();
            }
        }
    }

    void Application::restore()
    {
        if (m_surface)
        {
            m_surface->request_restore();
            for (auto const &[id, handler] : m_on_maximize_handlers)
            {
                if (handler)
                    handler(false);
            }
        }
    }

    bool Application::is_maximized() const
    {
        return m_surface && m_surface->is_maximized();
    }

    void Application::quit()
    {
        if (m_is_running)
        {
            m_is_running = false;
            for (auto const &[id, handler] : m_on_exit_handlers)
            {
                if (handler)
                    handler();
            }
        }
    }

    void Application::dispatch_events() {}

    size_t Application::add_on_start(std::function<void()> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_start_handlers[id] = handler;
        return id;
    }
    void Application::remove_on_start(size_t id)
    {
        m_on_start_handlers.erase(id);
    }

    size_t Application::add_on_exit(std::function<void()> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_exit_handlers[id] = handler;
        return id;
    }
    void Application::remove_on_exit(size_t id)
    {
        m_on_exit_handlers.erase(id);
    }

    size_t Application::add_on_resize(std::function<void(int, int)> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_resize_handlers[id] = handler;
        return id;
    }
    void Application::remove_on_resize(size_t id)
    {
        m_on_resize_handlers.erase(id);
    }

    size_t Application::add_on_maximize(std::function<void(bool)> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_maximize_handlers[id] = handler;
        return id;
    }
    void Application::remove_on_maximize(size_t id)
    {
        m_on_maximize_handlers.erase(id);
    }

    size_t Application::add_on_minimize(std::function<void()> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_minimize_handlers[id] = handler;
        return id;
    }
    void Application::remove_on_minimize(size_t id)
    {
        m_on_minimize_handlers.erase(id);
    }

} // namespace horizon
