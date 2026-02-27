#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/Widget.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <iostream>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

namespace horizon
{

    Application::Application(int w, int h)
    {
        m_wakeup_fd = eventfd(0, EFD_NONBLOCK);

        // Inicialización del sistema
        m_surface = std::make_unique<WaylandSurface>(w, h);
        m_surface->init();
        m_surface->set_event_listener(this);

        theme_manager = std::make_unique<ThemeManager>();

        theme_manager->when_change.connect(
            [this](EventContext &p)
            {
                std::cout << "Theme changed" << std::endl;
                this->invalidate();
            });
    }

    // Constructor de movimiento
    Application::Application(Application &&other) noexcept
        : m_is_running(other.m_is_running), m_root(std::move(other.m_root))
    {
        m_wakeup_fd = other.m_wakeup_fd;
        other.m_wakeup_fd = -1;
        other.m_is_running = false;
    }

    // Operador de asignación de movimiento
    Application &Application::operator=(Application &&other) noexcept
    {
        if (this != &other)
        {
            m_surface->free();
            if (m_wakeup_fd >= 0)
                close(m_wakeup_fd);

            m_is_running = other.m_is_running;
            m_root = std::move(other.m_root);
            m_wakeup_fd = other.m_wakeup_fd;

            other.m_is_running = false;
            other.m_wakeup_fd = -1;
        }
        return *this;
    }

    Application::~Application()
    {
        // Limpieza
        m_surface->free();
        if (m_wakeup_fd >= 0)
        {
            close(m_wakeup_fd);
        }
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
        invalidate();

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

        // Modifiers
        if (event.key == KEY_LEFTSHIFT || event.key == KEY_RIGHTSHIFT)
            m_modifiers |= SHIFT;
        if (event.key == KEY_LEFTCTRL || event.key == KEY_RIGHTCTRL)
            m_modifiers |= CTRL;
        if (event.key == KEY_LEFTALT || event.key == KEY_RIGHTALT)
            m_modifiers |= ALT;
        if (event.key == KEY_CAPSLOCK)
            m_modifiers ^= CAPSLOCK;

        // Key repeat management
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();

        if (m_repeat_key != event.key)
        {
            m_repeat_key = event.key;
            m_repeat_start_time = now;
            m_repeat_last_time = now;
            m_is_repeating = true;
        }

        Widget *target = m_focused ? m_focused : m_root.get();

        EventContext new_ev = {.sender = nullptr,
                               .type = EventType::KeyPress,
                               .button = event.key,
                               .stop_propagation = false,
                               .data = nullptr,
                               .eventX = 0,
                               .eventY = 0,
                               .key = event.key,
                               .modifiers = event.modifiers, // Use xkb-populated modifiers
                               .keysym = event.keysym,
                               .text = event.text};

        // Also update internal m_modifiers for backward compatibility/internal logic
        m_modifiers = event.modifiers;
        target->when_key_press.run(new_ev);
    }

    void Application::handle_key_release(const KeyEvent &event)
    {
        if (!m_root)
            return;

        // Modifiers
        if (event.key == KEY_LEFTSHIFT || event.key == KEY_RIGHTSHIFT)
            m_modifiers &= ~SHIFT;
        if (event.key == KEY_LEFTCTRL || event.key == KEY_RIGHTCTRL)
            m_modifiers &= ~CTRL;
        if (event.key == KEY_LEFTALT || event.key == KEY_RIGHTALT)
            m_modifiers &= ~ALT;

        if (event.key == m_repeat_key)
        {
            m_is_repeating = false;
            m_repeat_key = 0;
        }

        m_modifiers = event.modifiers;

        Widget *target = m_focused ? m_focused : m_root.get();

        EventContext new_ev = {.sender = nullptr,
                               .type = EventType::KeyRelease,
                               .button = event.key,
                               .stop_propagation = false,
                               .data = nullptr,
                               .eventX = 0,
                               .eventY = 0,
                               .key = event.key,
                               .modifiers = event.modifiers,
                               .keysym = event.keysym,
                               .text = event.text};
        target->when_key_release.run(new_ev);
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
                Widget *temp = m_hovered;
                while (temp)
                {
                    EventContext new_ev = {.sender = temp,
                                           .type = EventType::MouseLeave,
                                           .button = event.button,
                                           .stop_propagation = false,
                                           .data = nullptr,
                                           .eventX = m_pointer_x,
                                           .eventY = m_pointer_y,
                                           .key = 0,
                                           .modifiers = m_modifiers};
                    temp->when_mouse_leave.run(new_ev);
                    temp = temp->parent();
                }
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
                std::vector<Widget *> old_path;
                Widget *temp = m_hovered;
                while (temp)
                {
                    old_path.push_back(temp);
                    temp = temp->parent();
                }

                std::vector<Widget *> new_path;
                temp = under;
                while (temp)
                {
                    new_path.push_back(temp);
                    temp = temp->parent();
                }

                // Send MouseLeave to widgets in old path that are NOT in new path
                for (Widget *w : old_path)
                {
                    if (std::find(new_path.begin(), new_path.end(), w) == new_path.end())
                    {
                        EventContext leave_ev = {.sender = w,
                                                 .type = EventType::MouseLeave,
                                                 .button = event.button,
                                                 .stop_propagation = false,
                                                 .data = nullptr,
                                                 .eventX = m_pointer_x,
                                                 .eventY = m_pointer_y,
                                                 .key = 0,
                                                 .modifiers = m_modifiers};
                        w->when_mouse_leave.run(leave_ev);
                    }
                }

                // Send MouseEnter to widgets in new path that were NOT in old path
                for (auto it = new_path.rbegin(); it != new_path.rend(); ++it)
                {
                    Widget *w = *it;
                    if (std::find(old_path.begin(), old_path.end(), w) == old_path.end())
                    {
                        EventContext enter_ev = {.sender = w,
                                                 .type = EventType::MouseEnter,
                                                 .button = event.button,
                                                 .stop_propagation = false,
                                                 .data = nullptr,
                                                 .eventX = m_pointer_x,
                                                 .eventY = m_pointer_y,
                                                 .key = 0,
                                                 .modifiers = m_modifiers};
                        w->when_mouse_enter.run(enter_ev);
                    }
                }

                m_hovered = under;

                if (m_hovered)
                {
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
                                   .data = nullptr,
                                   .eventX = (double)event.x,
                                   .eventY = (double)event.y,
                                   .key = 0,
                                   .modifiers = m_modifiers};
            m_pressed->when_mouse_drag.run(new_ev);
        }
        else if (m_hovered)
        {
            EventContext new_ev = {.sender = m_hovered,
                                   .type = EventType::MouseHover,
                                   .button = event.button,
                                   .stop_propagation = false,
                                   .data = nullptr,
                                   .eventX = (double)event.x,
                                   .eventY = (double)event.y,
                                   .key = 0,
                                   .modifiers = m_modifiers};
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

            // Update focus
            if (m_focused != under)
            {
                if (m_focused)
                    m_focused->set_focus(false);
                m_focused = under;
                m_focused->set_focus(true);
            }

            EventContext new_ev = {.sender = m_pressed,
                                   .type = EventType::MousePress,
                                   .button = event.button,
                                   .stop_propagation = false,
                                   .data = nullptr,
                                   .eventX = (double)event.x,
                                   .eventY = (double)event.y,
                                   .key = 0,
                                   .modifiers = m_modifiers};

            m_pressed->when_mouse_press.run(new_ev);
            // m_pressed->on_mouse_press(event.button);
        }
        else
        {
            if (m_focused)
            {
                m_focused->set_focus(false);
                m_focused = nullptr;
            }
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
                                   .data = nullptr,
                                   .eventX = (double)event.x,
                                   .eventY = (double)event.y,
                                   .key = 0,
                                   .modifiers = m_modifiers};

            m_pressed->when_mouse_release.run(new_ev);
            m_pressed = nullptr;
        }
    }

    void Application::set_root(std::unique_ptr<Widget> root)
    {
        m_root = std::move(root);
        if (m_root)
        {
            m_root->set_application_recursive(this);
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

        struct pollfd fds[2];
        fds[0].fd = wl_display_get_fd(m_surface->display());
        fds[0].events = POLLIN;
        fds[1].fd = m_wakeup_fd;
        fds[1].events = POLLIN;

        while (m_is_running)
        {
            wl_display_dispatch_pending(m_surface->display());

            if (m_full_repaint && m_root)
            {
                m_full_repaint = false;
                m_dirty_widgets.clear();

                CairoGraphicContext ctx(m_surface->data(), m_surface->width(), m_surface->height());
                m_root->render(ctx);

                wl_surface_attach(m_surface->surface(), m_surface->buffer(), 0, 0);
                wl_surface_damage(m_surface->surface(), 0, 0, m_surface->width(),
                                  m_surface->height());
                wl_surface_commit(m_surface->surface());
            }
            else if (!m_dirty_widgets.empty() && m_root)
            {
                std::vector<Widget *> current_dirty;
                std::swap(current_dirty, m_dirty_widgets);

                CairoGraphicContext ctx(m_surface->data(), m_surface->width(), m_surface->height());

                for (Widget *w : current_dirty)
                {
                    ctx.save();
                    ctx.clip(w->x(), w->y(), w->width(), w->height());
                    m_root->render(ctx);
                    ctx.restore();

                    wl_surface_damage(m_surface->surface(), w->x(), w->y(), w->width(),
                                      w->height());
                }

                ctx.flush();
                wl_surface_attach(m_surface->surface(), m_surface->buffer(), 0, 0);
                wl_surface_commit(m_surface->surface());
            }

            wl_display_flush(m_surface->display());

            uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

            int timeout = -1;
            if (m_is_repeating)
                timeout = 10;
            else if (!m_timers.empty())
            {
                uint64_t next_expiry = 0;
                for (const auto &[id, timer] : m_timers)
                {
                    if (next_expiry == 0 || timer.next_expiry < next_expiry)
                        next_expiry = timer.next_expiry;
                }

                if (next_expiry <= now)
                    timeout = 0;
                else
                    timeout = (int)(next_expiry - now);
            }
            else if (m_focused)
                timeout = 250; // Heartbeat for blinking

            int ret = poll(fds, 2, timeout);

            now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();

            // Handle timers
            std::vector<size_t> to_run;
            for (auto const &[id, timer] : m_timers)
            {
                if (now >= timer.next_expiry)
                {
                    to_run.push_back(id);
                }
            }

            for (size_t id : to_run)
            {
                if (m_timers.count(id))
                {
                    // Copy callback to avoid use-after-free if callback removes the timer
                    auto callback = m_timers[id].callback;
                    m_timers[id].next_expiry = now + m_timers[id].interval_ms;

                    if (callback)
                        callback();
                }
            }

            if (ret == 0 && m_focused && !m_is_repeating && m_timers.empty())
            {
                m_focused->invalidate();
            }

            if (m_is_repeating)
            {
                uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count();
                if (now - m_repeat_start_time >= m_repeat_delay)
                {
                    if (now - m_repeat_last_time >= m_repeat_rate)
                    {
                        KeyEvent ev;
                        ev.type = KeyEvent::Type::Press;
                        ev.key = m_repeat_key;
                        handle_key_press(ev);
                        m_repeat_last_time = now;
                    }
                }
            }

            if (ret > 0)
            {
                if (fds[0].revents & POLLIN)
                {
                    wl_display_dispatch(m_surface->display());
                }
                if (fds[1].revents & POLLIN)
                {
                    uint64_t val;
                    if (read(m_wakeup_fd, &val, sizeof(val)) < 0)
                    {
                        // ignore error
                    }
                }
            }
        }

        quit();
    }

    void Application::invalidate(Widget *widget)
    {
        if (!widget)
        {
            m_full_repaint = true;
        }
        else
        {
            if (std::find(m_dirty_widgets.begin(), m_dirty_widgets.end(), widget) ==
                m_dirty_widgets.end())
            {
                m_dirty_widgets.push_back(widget);
            }
        }
        wakeup();
    }

    void Application::wakeup()
    {
        if (m_wakeup_fd >= 0)
        {
            uint64_t val = 1;
            if (write(m_wakeup_fd, &val, sizeof(val)) < 0)
            {
                // ignore error
            }
        }
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

    size_t Application::add_timer(int ms, std::function<void()> callback)
    {
        size_t id = m_next_timer_id++;
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();

        Timer t;
        t.id = id;
        t.interval_ms = ms;
        t.next_expiry = now + ms;
        t.callback = callback;

        m_timers[id] = t;
        wakeup(); // Wake up the loop to reconsider timeout
        return id;
    }

    void Application::stop_timer(size_t id)
    {
        m_timers.erase(id);
    }

} // namespace horizon
