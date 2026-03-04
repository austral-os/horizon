#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/Widget.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/ClientMenu.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/Menu.hpp>
#include <horizon/Window.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <iostream>
#include <linux/input-event-codes.h>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

namespace horizon
{

    Application::Application(int w, int h) : Application(w, h, false) {}

    Application::Application(int w, int h, bool defer_init)
    {
        m_wakeup_fd = eventfd(0, EFD_NONBLOCK);

        // Inicialización del sistema
        m_surface = std::make_unique<WaylandSurface>(w, h);
        if (!defer_init)
        {
            m_surface->init();
        }
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
        m_full_repaint = true;
        invalidate();

        for (auto const &[id, handler] : m_on_resize_handlers)
        {
            if (handler)
                handler(width, height);
        }
    }

    void Application::on_activated(bool active)
    {
        m_is_activated = active;

        if (m_client_menu)
        {
            if (active)
            {
                m_client_menu->set_global_menu(m_global_menus);
            }
            else
            {
                m_client_menu->set_global_menu({});
            }
        }

        EventContext ev;
        ev.sender = this;
        ev.type = active ? EventType::AppActivated : EventType::AppDeactivated;
        if (active)
        {
            when_activated.run(ev);
        }
        else
        {
            when_deactivated.run(ev);
        }
    }

    void Application::set_global_menu(const std::vector<Menu *> &menus)
    {
        m_global_menus = menus;
        if (!m_client_menu)
        {
            m_client_menu = std::make_shared<ClientMenu>();
        }

        if (m_is_activated)
        {
            m_client_menu->set_global_menu(m_global_menus);
        }
    }

    void Application::on_key_event(const KeyEvent &event)
    {
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

    void Application::on_modifiers_event(uint32_t modifiers)
    {
        m_modifiers = modifiers;
    }

    void Application::handle_key_press(const KeyEvent &event)
    {
        if (event.key == KEY_ESC)
        {
            quit();
            return;
        }

        // Key repeat management — only reset if this is a NEW key being pressed (not a synthetic
        // repeat)
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();

        if (m_repeat_key != event.key)
        {
            m_repeat_key = event.key;
            m_repeat_event = event; // Store full event (keysym, text, etc.) for replay
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

        // We no longer update m_modifiers here; on_modifiers_event is the sole source of truth
        target->when_key_press.run(new_ev);
    }

    void Application::handle_key_release(const KeyEvent &event)
    {
        if (!m_root)
            return;

        if (event.key == m_repeat_key)
        {
            m_is_repeating = false;
            m_repeat_key = 0;
        }

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

            Widget *temp = m_pressed;
            while (temp)
            {
                new_ev.sender = temp;
                temp->when_mouse_drag.run(new_ev);
                if (new_ev.stop_propagation)
                    break;
                temp = temp->parent();
            }
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

            Widget *temp = m_hovered;
            while (temp)
            {
                new_ev.sender = temp;
                temp->when_mouse_hover.run(new_ev);
                temp->when_mouse_move.run(new_ev);
                if (new_ev.stop_propagation)
                    break;
                temp = temp->parent();
            }
        }

        // Push final cursor state to surface
        if (m_resize_edge != 0)
        {
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
        else if (m_hovered)
        {
            m_surface->set_cursor(m_hovered->cursor_type());
        }
        else
        {
            m_surface->set_cursor(CursorType::Default);
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

            Widget *temp = m_pressed;
            while (temp)
            {
                new_ev.sender = temp;
                temp->when_mouse_press.run(new_ev);
                if (new_ev.stop_propagation)
                    break;
                temp = temp->parent();
            }
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

            Widget *temp = m_pressed;
            while (temp)
            {
                new_ev.sender = temp;
                temp->when_mouse_release.run(new_ev);
                if (new_ev.stop_propagation)
                    break;
                temp = temp->parent();
            }
            m_pressed = nullptr;
        }
    }

    void Application::set_root(std::unique_ptr<Widget> root)
    {
        m_root = std::move(root);
        if (m_root)
        {
            m_root->set_application_recursive(this);
            m_root->set_size(m_surface->width(), m_surface->height());
            m_full_repaint = true;
            invalidate();
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

        notify_app_manager("app_started");

        struct pollfd fds[2];
        fds[0].fd = wl_display_get_fd(m_surface->display());
        fds[0].events = POLLIN;
        fds[1].fd = m_wakeup_fd;
        fds[1].events = POLLIN;

        while (m_is_running)
        {
            if (!m_ipc_subscriber)
            {
                m_ipc_subscriber = std::make_unique<IpcClient>("/tmp/horizon_apps.sock");
                m_ipc_subscriber->subscribe(
                    "{\"type\": \"subscribe\"}",
                    [this](const std::string &msg)
                    {
                        try
                        {
                            auto j = nlohmann::json::parse(msg);
                            if (j.value("type", "unknown") == "app_signal")
                            {
                                int target_pid = j.value("target_pid", -1);
                                if (target_pid == getpid())
                                {
                                    std::string signal = j.value("signal", "unknown");
                                    std::string token = j.value("token", "");
                                    std::cout << "[APP] Received remote signal: " << signal
                                              << " (token: " << (token.empty() ? "none" : "present")
                                              << ", posting task)" << std::endl;
                                    this->post_task(
                                        [this, signal, token]()
                                        {
                                            if (signal == "maximize")
                                                this->maximize();
                                            else if (signal == "minimize")
                                                this->minimize();
                                            else if (signal == "restore")
                                                this->restore(token);
                                            else if (signal == "close")
                                                this->on_close();
                                            else if (signal == "fullscreen")
                                                this->fullscreen();
                                            else if (signal == "unfullscreen")
                                                this->unfullscreen();
                                        });
                                }
                            }
                        }
                        catch (...)
                        {
                        }
                    });
            }

            wl_display_dispatch_pending(m_surface->display());

            // Frame rate limiter: compute current time and skip rendering if
            // less than 16ms (~60fps) has elapsed since the last Wayland commit.
            // Without this, rapid drag events generate 100+ commits/sec which
            // causes the compositor to see the buffer being written while it's
            // still reading it → flicker.
            {
                uint64_t frame_now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now().time_since_epoch())
                                         .count();
                static constexpr uint64_t FRAME_MS = 16; // ~60fps cap

                bool has_pending = m_full_repaint || !m_dirty_widgets.empty();

                if (has_pending && m_surface->buffer() && m_surface->is_configured() &&
                    (frame_now - m_last_commit_time) >= FRAME_MS)
                {
                    if (m_full_repaint && m_root)
                    {
                        m_full_repaint = false;
                        m_dirty_widgets.clear();
                        CairoGraphicContext ctx(m_surface->data(), m_surface->width(),
                                                m_surface->height());
                        // For transparent surfaces (OverlayApplication), clear the entire surface
                        // to remove stale pixels. Without this, popGroup's OVER compositing
                        // preserves old pixels from hidden widgets.
                        if (is_transparent_surface())
                        {
                            ctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                            ctx.clearRect(0, 0, m_surface->width(), m_surface->height());
                        }
                        ctx.pushGroup();
                        m_root->render(ctx, 0, 0, m_surface->width(), m_surface->height(), true);
                        ctx.popGroup();
                        ctx.flush();

                        wl_surface_damage(m_surface->surface(), 0, 0, m_surface->width(),
                                          m_surface->height());
                        wl_surface_attach(m_surface->surface(), m_surface->buffer(), 0, 0);
                        wl_surface_commit(m_surface->surface());
                        m_last_commit_time = frame_now;
                    }
                    else if (!m_dirty_widgets.empty() && m_root)
                    {
                        std::vector<Widget *> current_dirty;
                        std::swap(current_dirty, m_dirty_widgets);

                        CairoGraphicContext ctx(m_surface->data(), m_surface->width(),
                                                m_surface->height());

                        for (Widget *w : current_dirty)
                        {
                            // 1. Precise damage reporting
                            wl_surface_damage(m_surface->surface(), w->x(), w->y(), w->width(),
                                              w->height());

                            // 2. Individual rendering pass with absolute clip and force=true
                            // Force is necessary to ensure parent clears background for the dirty
                            // widget area.
                            ctx.save();
                            ctx.clip(w->x(), w->y(), w->width(), w->height());

                            // For transparent surfaces, we must clear the destination region
                            // before compositing the new widget tree,
                            // otherwise old pixels remain underneath
                            if (is_transparent_surface())
                            {
                                ctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                                ctx.clearRect(w->x(), w->y(), w->width(), w->height());
                            }

                            ctx.pushGroup();
                            m_root->render(ctx, w->x(), w->y(), w->width(), w->height(), true);
                            ctx.popGroup();
                            ctx.restore();
                        }

                        ctx.flush();
                        wl_surface_attach(m_surface->surface(), m_surface->buffer(), 0, 0);
                        wl_surface_commit(m_surface->surface());
                        m_last_commit_time = frame_now;
                    }
                }
            }

            wl_display_flush(m_surface->display());

            uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

            // Calculate poll() timeout — take the minimum of: repeating, timer expiry, blink
            // heartbeat
            int timeout = -1;
            if (m_is_repeating)
            {
                timeout = 10;
            }
            else
            {
                // Check nearest timer expiry
                if (!m_timers.empty())
                {
                    uint64_t next_expiry = 0;
                    for (const auto &[id, timer] : m_timers)
                    {
                        if (next_expiry == 0 || timer.next_expiry < next_expiry)
                            next_expiry = timer.next_expiry;
                    }
                    int timer_ms = (next_expiry <= now) ? 0 : (int)(next_expiry - now);
                    timeout = (timeout == -1) ? timer_ms : std::min(timeout, timer_ms);
                }

                // Cursor blink heartbeat: always needed when something is focused
                if (m_focused)
                {
                    int blink_ms = 500 - (int)(now - m_blink_last_time);
                    if (blink_ms < 0)
                        blink_ms = 0;
                    timeout = (timeout == -1) ? blink_ms : std::min(timeout, blink_ms);
                }
            }

            int ret = poll(fds, 2, timeout);

            now = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();

            // Handle timers
            std::vector<size_t> to_run;
            for (auto const &[id, timer] : m_timers)
            {
                if (now >= timer.next_expiry)
                    to_run.push_back(id);
            }

            for (size_t id : to_run)
            {
                if (m_timers.count(id))
                {
                    auto &timer = m_timers[id];
                    auto callback = timer.callback;
                    bool repeat = timer.repeat;

                    if (repeat)
                    {
                        timer.next_expiry = now + timer.interval_ms;
                    }
                    else
                    {
                        m_timers.erase(id);
                    }

                    if (callback)
                        callback();
                }
            }

            // Trigger cursor blink independently of timers
            if (m_focused && !m_is_repeating && (now - m_blink_last_time >= 500))
            {
                m_blink_last_time = now;
                m_focused->invalidate();
            }

            if (ret > 0)
            {
                // Always dispatch Wayland events FIRST, before anything else,
                // so physical key events update m_is_repeating state before the repeat check.
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

            // Execute posted tasks
            {
                std::deque<std::function<void()>> tasks;
                {
                    std::lock_guard<std::mutex> lock(m_task_mutex);
                    std::swap(tasks, m_task_queue);
                }
                for (auto &task : tasks)
                {
                    if (task)
                    {
                        std::cout << "[APP] Executing posted task..." << std::endl;
                        task();
                    }
                }
            }

            // Key repeat check — runs on every loop iteration when a key is held
            if (m_is_repeating)
            {
                now = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
                if (now - m_repeat_start_time >= m_repeat_delay)
                {
                    if (now - m_repeat_last_time >= m_repeat_rate)
                    {
                        // Replay the full stored event but use CURRENT modifiers
                        KeyEvent repeat_ev = m_repeat_event;
                        repeat_ev.modifiers = m_modifiers;
                        handle_key_press(repeat_ev);
                        m_repeat_last_time = now;
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

    void Application::post_task(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_task_queue.push_back(std::move(task));
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
            m_is_minimized = false;
            notify_app_manager("app_started"); // Notify state change
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
            std::cout << "[APP] Minimizing window..." << std::endl;
            m_was_maximized_before_minimize = is_maximized();
            m_surface->request_minimize();
            m_is_minimized = true;
            notify_app_manager("app_started"); // Notify state change
            for (auto const &[id, handler] : m_on_minimize_handlers)
            {
                if (handler)
                    handler();
            }
        }
    }

    void Application::restore(const std::string &token)
    {
        if (m_surface)
        {
            if (!token.empty())
            {
                std::cout << "[APP] Using activation token for restore" << std::endl;
                m_surface->activate(token);
            }

            // If it was maximized before, request maximize again.
            // Otherwise, just request restore (unminimize).
            if (m_was_maximized_before_minimize)
            {
                m_surface->request_maximize();
            }
            else
            {
                m_surface->request_restore();
            }

            wl_display_flush(m_surface->display());

            m_is_minimized = false;
            notify_app_manager("app_started"); // Notify state change
            for (auto const &[id, handler] : m_on_maximize_handlers)
            {
                if (handler)
                    handler(m_was_maximized_before_minimize);
            }
        }
    }

    WaylandSurface *Application::w_surface() const
    {
        return m_surface.get();
    }

    bool Application::is_maximized() const
    {
        return m_surface && m_surface->is_maximized();
    }

    void Application::fullscreen()
    {
        if (m_surface)
        {
            m_surface->request_fullscreen();
        }
    }

    void Application::unfullscreen()
    {
        if (m_surface)
        {
            m_surface->request_unfullscreen();
        }
    }

    bool Application::is_fullscreen() const
    {
        return m_surface && m_surface->is_fullscreen();
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
            notify_app_manager("app_stopped");
        }
    }

    void Application::notify_app_manager(const std::string &type)
    {
        // Capture necessary data to avoid use-after-free in the thread
        std::string app_id = m_app_id;
        std::string name = m_name;
        std::string icon = m_icon_name;
        bool show_dock = m_show_in_dock;
        bool show_tray = m_show_in_system_tray;
        bool is_min = m_is_minimized;
        pid_t pid = getpid();

        std::thread(
            [app_id, name, icon, show_dock, show_tray, is_min, pid, type]()
            {
                try
                {
                    nlohmann::json msg;
                    msg["type"] = type;
                    msg["app_id"] = app_id;
                    msg["name"] = name;
                    msg["icon"] = icon;
                    msg["show_in_dock"] = show_dock;
                    msg["show_in_system_tray"] = show_tray;
                    msg["is_minimized"] = is_min;
                    msg["pid"] = pid;

                    IpcClient client("/tmp/horizon_apps.sock");
                    // Simple retry logic
                    for (int i = 0; i < 3; ++i)
                    {
                        if (client.send(msg.dump()))
                            break;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
                catch (...)
                {
                }
            })
            .detach();
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

    size_t Application::add_timer(int ms, std::function<void()> callback, bool repeat)
    {
        size_t id = m_next_timer_id++;
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();

        Timer t;
        t.id = id;
        t.interval_ms = ms;
        t.next_expiry = now + ms;
        t.repeat = repeat;
        t.callback = callback;

        m_timers[id] = t;
        wakeup(); // Wake up the loop to reconsider timeout
        return id;
    }

    void Application::stop_timer(size_t id)
    {
        m_timers.erase(id);
    }

    void Application::unregister_widget(Widget *widget)
    {
        if (!widget)
            return;

        // Remove from dirty list
        m_dirty_widgets.erase(std::remove(m_dirty_widgets.begin(), m_dirty_widgets.end(), widget),
                              m_dirty_widgets.end());

        // Clear focused/hovered/pressed if this widget is going away
        if (m_focused == widget)
            m_focused = nullptr;
        if (m_hovered == widget)
            m_hovered = nullptr;
        if (m_pressed == widget)
            m_pressed = nullptr;
    }

    void Application::on_close()
    {
        EventContext ev;
        ev.sender = this;
        ev.type = EventType::AppExit;

        when_close.run(ev);

        if (!ev.stop_propagation)
        {
            if (m_client_menu)
            {
                m_client_menu->set_global_menu({}); // Clear global menu before exit
            }
            quit();
        }
    }

} // namespace horizon
