#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/ClientMenu.hpp"
#include "horizon/IpcClient.hpp"
#include "horizon/LabwcCompositorContext.hpp"
#include "horizon/Menu.hpp"
#include "horizon/WayfireCompositorContext.hpp"
#include <GLES2/gl2.h>
#include <algorithm>
#include <glib-object.h>
#include <horizon/Logger.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <linux/input-event-codes.h>
#include <memory>
#include <poll.h>
#include <set>
#include <sys/eventfd.h>
#include <unistd.h>

namespace horizon
{

    static const char *VERTEX_SHADER = "attribute vec3 position;\n"
                                       "attribute vec2 texcoord;\n"
                                       "varying vec2 v_texcoord;\n"
                                       "uniform mat4 u_mvp;\n"
                                       "void main() {\n"
                                       "    gl_Position = u_mvp * vec4(position, 1.0);\n"
                                       "    v_texcoord = texcoord;\n"
                                       "}\n";

    static const char *FRAGMENT_SHADER =
        "precision mediump float;\n"
        "varying vec2 v_texcoord;\n"
        "uniform sampler2D u_texture;\n"
        "uniform float u_opacity;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(u_texture, v_texcoord).bgra * u_opacity;\n"
        "}\n";

    WaylandWindow::WaylandWindow(std::string app_id, int w, int h, bool defer_init)
        : m_app_id(app_id)
    {
        // Inicialización del sistema
        m_surface = std::make_unique<WaylandSurface>(w, h);
        if (!defer_init)
        {
            m_surface->init_display();
            m_surface->setup_xdg_toplevel(m_name, m_app_id);
        }
        m_surface->set_event_listener(this);

        // Detect current compositor
        const char *xdg_current_desktop = std::getenv("XDG_CURRENT_DESKTOP");
        std::string desktop = xdg_current_desktop ? xdg_current_desktop : "";
        std::transform(desktop.begin(), desktop.end(), desktop.begin(), ::tolower);

        LOG_INFO << "[APP] Detecting compositor (XDG_CURRENT_DESKTOP=" << desktop << ")";

        if (desktop.find("wayfire") != std::string::npos ||
            desktop.find("hzn-wayfire") != std::string::npos)
        {
            LOG_INFO << "[APP] Recognized Wayfire compositor, using WayfireCompositorContext";
            m_compositor_context = std::make_unique<WayfireCompositorContext>(this);
        }
        else if (desktop.find("labwc") != std::string::npos ||
                 desktop.find("hzn-labwc") != std::string::npos)
        {
            LOG_INFO << "[APP] Recognized Labwc compositor, using LabwcCompositorContext";
            m_compositor_context = std::make_unique<LabwcCompositorContext>(this);
        }
        else
        {
            LOG_INFO << "[APP] Unknown or generic compositor, defaulting to LabwcCompositorContext "
                        "(XDG-Shell)";
            m_compositor_context = std::make_unique<LabwcCompositorContext>(this);
        }

        theme_manager = std::make_unique<ThemeManager>();

        theme_manager->when_change.connect(
            [this](ThemeEventContext &p)
            {
                LOG_INFO << "Theme changed";
                this->invalidate();
            });

        signal_manager.connect("quit",
                               [this](SignalContext &p)
                               {
                                   LOG_INFO << "[SIGNAL] Quit signal received" << std::endl;
                                   this->post_task([this]() { this->on_close(); });
                               });

        signal_manager.connect("fullscreen",
                               [this](SignalContext &p)
                               {
                                   {
                                       LOG_INFO
                                           << "[SIGNAL] Fullscreen signal received, toggling state"
                                           << std::endl;
                                       this->post_task(
                                           [this]()
                                           {
                                               if (this->is_fullscreen())
                                                   this->unfullscreen();
                                               else
                                                   this->fullscreen();
                                           });
                                   }
                               });

        // Initialize logger
        Logger::instance().init(m_app_id);
        LOG_INFO << "Application started: " << m_name << " (" << m_app_id << ")";

        m_wakeup_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (m_wakeup_fd < 0)
        {
            LOG_ERROR << "[APP] Failed to create eventfd: " << strerror(errno);
        }

        m_app_menu = std::make_unique<Menu>();
    };

    WaylandWindow::~WaylandWindow()
    {
        // Cleanup image cache
        for (auto const &[path, handle] : m_svg_cache)
        {
            if (handle)
                g_object_unref(handle);
        }
        for (auto const &[path, surface] : m_surface_cache)
        {
            if (surface)
                cairo_surface_destroy(static_cast<cairo_surface_t *>(surface));
        }

        // Limpieza
        m_surface->free();

        if (m_wakeup_fd >= 0)
        {
            close(m_wakeup_fd);
        }
    }

    void WaylandWindow::send_remote_signal(int target_pid, const std::string &signal,
                                           const std::string &token)
    {
        std::thread(
            [target_pid, signal, token]()
            {
                try
                {
                    nlohmann::json msg;
                    msg["type"] = "send_signal";
                    msg["target_pid"] = target_pid;
                    msg["signal"] = signal;
                    if (!token.empty())
                    {
                        msg["token"] = token;
                    }

                    IpcClient client("/tmp/horizon_session.sock");
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

    size_t WaylandWindow::add_on_start(std::function<void()> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_start_handlers[id] = handler;
        return id;
    }
    void WaylandWindow::remove_on_start(size_t id)
    {
        m_on_start_handlers.erase(id);
    }

    size_t WaylandWindow::add_on_exit(std::function<void()> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_exit_handlers[id] = handler;
        return id;
    }
    void WaylandWindow::remove_on_exit(size_t id)
    {
        m_on_exit_handlers.erase(id);
    }

    size_t WaylandWindow::add_on_resize(std::function<void(int, int)> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_resize_handlers[id] = handler;
        return id;
    }
    void WaylandWindow::remove_on_resize(size_t id)
    {
        m_on_resize_handlers.erase(id);
    }

    size_t WaylandWindow::add_on_maximize(std::function<void(bool)> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_maximize_handlers[id] = handler;
        return id;
    }
    void WaylandWindow::remove_on_maximize(size_t id)
    {
        m_on_maximize_handlers.erase(id);
    }

    size_t WaylandWindow::add_on_minimize(std::function<void()> handler)
    {
        size_t id = m_next_app_handler_id++;
        m_on_minimize_handlers[id] = handler;
        return id;
    }
    void WaylandWindow::remove_on_minimize(size_t id)
    {
        m_on_minimize_handlers.erase(id);
    }

    void WaylandWindow::on_close()
    {
        AppEventContext ev;
        ev.sender = this;

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

    CompositorContext &WaylandWindow::get_compositor_context() const
    {
        return *m_compositor_context;
    }

    void WaylandWindow::on_foreign_toplevel_event()
    {
        AppListEventContext ctx;
        if (m_surface)
        {
            std::set<std::string> seen_keys;

            auto process_ft = [&](const WaylandSurface::ForeignToplevel &ft, uintptr_t instance_id)
            {
                // Create a unique key to deduplicate between protocols.
                // Usually app_id is enough, but some apps might have empty app_id but unique
                // titles.
                std::string key = ft.app_id + "|" + ft.title;
                if (seen_keys.find(key) != seen_keys.end())
                    return;

                seen_keys.insert(key);

                ApplicationInfo info;
                info.app_id = ft.app_id;
                info.title = ft.title;
                info.instance_id = instance_id;
                info.is_active = ft.active;
                info.is_minimized = ft.minimized;
                info.show_in_dock = true;
                ctx.apps.push_back(info);
            };

            const auto &foreigns = m_surface->get_foreign_toplevels();
            for (const auto &pair : foreigns)
            {
                process_ft(pair.second, reinterpret_cast<uintptr_t>(pair.first));
            }

            const auto &ext_foreigns = m_surface->get_ext_foreign_toplevels();
            for (const auto &pair : ext_foreigns)
            {
                process_ft(pair.second, reinterpret_cast<uintptr_t>(pair.first));
            }
        }
        when_foreign_update.run(ctx);
    }

    void WaylandWindow::initialize()
    {
        if (m_surface->is_configured())
            return;

        m_surface->init_display();
        m_surface->setup_xdg_toplevel(m_name, m_app_id);
    }

    void WaylandWindow::run()
    {

        m_is_running = true;

        init_global_menu();

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
                m_ipc_subscriber = std::make_unique<IpcClient>("/tmp/horizon_session.sock");
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
                                    LOG_INFO << "[APP] Received remote signal: " << signal
                                             << " (token: " << (token.empty() ? "none" : "present")
                                             << ", posting task)";
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
                            else if (j.value("type", "unknown") == "menu_item_clicked")
                            {
                                int receiver_pid = j.value("receiver_pid", -1);
                                if (receiver_pid == getpid())
                                {
                                    std::string item_id = j.value("id", "");
                                    SignalContext signal_ctx;
                                    signal_ctx.signal = item_id;
                                    signal_ctx.sender = nullptr;
                                    signal_ctx.data = nullptr;
                                    signal_manager.emit(item_id, signal_ctx);
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

                if (has_pending && !m_is_minimized)
                {
                    // Only render if not minimized to avoid hanging in
                    // eglSwapBuffers/wl_display_dispatch when the compositor might not be giving us
                    // frame callbacks.

                    if (m_surface->is_configured() && (frame_now - m_last_commit_time) >= FRAME_MS)
                    {
                        if (is_transparent_surface())
                            glClearColor(0, 0, 0, 0);
                        else
                            glClearColor(0, 0, 0, 1);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        m_gl_queue.clear();

                        if (m_root)
                        {
                            m_full_repaint = false;
                            m_dirty_widgets.clear();

                            CairoGraphicContext ctx(this, m_surface->data(), m_surface->width(),
                                                    m_surface->height());

                            if (is_transparent_surface())
                            {
                                ctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                                ctx.clearRect(0, 0, m_surface->width(), m_surface->height());
                            }

                            ctx.pushGroup();
                            m_root->render(ctx, 0, 0, m_surface->width(), m_surface->height(),
                                           true);
                            ctx.popGroup();
                            ctx.flush();

                            render_gl_ui();
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
                if (ret < 0)
                {
                    if (errno == EINTR)
                        continue;
                    LOG_ERROR << "[APP] poll() error: " << strerror(errno);
                    m_is_running = false;
                    break;
                }

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
                        if (wl_display_dispatch(m_surface->display()) == -1)
                        {
                            LOG_ERROR << "[APP] wl_display_dispatch() failed, exiting loop.";
                            m_is_running = false;
                            break;
                        }
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
                            LOG_INFO << "[APP] Executing posted task...";
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
        }
    }

    void WaylandWindow::set_global_menu(const std::vector<Menu *> &menus)
    {
        m_global_menus = menus;
    }

    void WaylandWindow::init_global_menu()
    {

        m_app_menu->set_title(m_name);
        m_app_menu->set_bold(true);
        m_app_menu->add_item("Preferencias", "Ctrl+,");
        auto *fullscreen_item = m_app_menu->add_item("Pantalla completa", "F11");
        fullscreen_item->set_id("fullscreen");
        m_app_menu->add_separator();
        auto *global_quit = m_app_menu->add_item("Salir", "Ctrl+Q");
        global_quit->set_id("quit");

        auto mnu = m_app_menu.get();
        if (std::find(m_global_menus.begin(), m_global_menus.end(), mnu) == m_global_menus.end())
        {
            m_global_menus.insert(m_global_menus.begin(), mnu);
        }

        if (!m_client_menu)
        {
            m_client_menu = std::make_shared<ClientMenu>();
        }

        if (m_is_activated)
        {
            m_client_menu->set_global_menu(m_global_menus);
        }
    }

    void WaylandWindow::add_menu(std::unique_ptr<Menu> menu)
    {
        if (!menu)
            return;

        Menu *ptr = menu.get();
        m_menues.push_back(std::move(menu));
        m_global_menus.push_back(ptr);

        if (m_client_menu && m_is_activated)
        {
            m_client_menu->set_global_menu(m_global_menus);
        }
    }

    void WaylandWindow::delete_menu(const std::string &title)
    {
        auto it = std::find_if(m_menues.begin(), m_menues.end(),
                               [&title](const std::unique_ptr<Menu> &m)
                               { return m->title() == title; });

        if (it != m_menues.end())
        {
            Menu *ptr = it->get();
            m_global_menus.erase(std::remove(m_global_menus.begin(), m_global_menus.end(), ptr),
                                 m_global_menus.end());
            m_menues.erase(it);

            if (m_client_menu && m_is_activated)
            {
                m_client_menu->set_global_menu(m_global_menus);
            }
        }
    }

    void WaylandWindow::delete_all_menues()
    {
        for (auto &menu : m_menues)
        {
            Menu *ptr = menu.get();
            m_global_menus.erase(std::remove(m_global_menus.begin(), m_global_menus.end(), ptr),
                                 m_global_menus.end());
        }
        m_menues.clear();

        if (m_client_menu && m_is_activated)
        {
            m_client_menu->set_global_menu(m_global_menus);
        }
    }

    void WaylandWindow::set_app_menu(std::unique_ptr<Menu> menu)
    {
        if (m_app_menu)
        {
            Menu *old_ptr = m_app_menu.get();
            m_global_menus.erase(
                std::remove(m_global_menus.begin(), m_global_menus.end(), old_ptr),
                m_global_menus.end());
        }

        m_app_menu = std::move(menu);

        if (m_app_menu)
        {
            m_global_menus.insert(m_global_menus.begin(), m_app_menu.get());
        }

        if (m_client_menu && m_is_activated)
        {
            m_client_menu->set_global_menu(m_global_menus);
        }
    }

    void WaylandWindow::on_pointer_event(const PointerEvent &event)
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
        case PointerEvent::Type::Scroll:
            handle_wheel(event);
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

    void WaylandWindow::on_resize(int width, int height)
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

    void WaylandWindow::on_activated(bool active)
    {
        m_is_activated = active;
        LOG_INFO << "[APP] on_activated: " << active << " (PID: " << getpid() << ")";

        if (m_client_menu)
        {
            if (active)
            {
                LOG_INFO << "[APP] Sending global menu to panel.";
                m_client_menu->set_global_menu(m_global_menus);
            }
            else
            {
                LOG_INFO << "[APP] Clearing global menu from panel.";
                m_client_menu->set_global_menu({});
            }
        }

        if (active && m_is_minimized)
        {
            LOG_INFO << "[APP] Window activated while minimized, marking as restored and "
                        "triggering repaint (PID: "
                     << getpid() << ")";
            m_is_minimized = false;
            m_full_repaint = true;
            notify_app_manager("window_state_changed");
            invalidate();
        }

        AppEventContext ev;
        ev.sender = this;
        if (active)
        {
            when_activated.run(ev);
        }
        else
        {
            when_deactivated.run(ev);
        }
    }

    void WaylandWindow::on_key_event(const KeyEvent &event)
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

    void WaylandWindow::on_modifiers_event(uint32_t modifiers)
    {
        m_modifiers = modifiers;
    }

    void WaylandWindow::handle_key_press(const KeyEvent &event)
    {
        if (event.key == KEY_ESC)
        {
            quit();
            return;
        }

        // Key repeat management — only reset if this is a NEW key being pressed (not a
        // synthetic repeat)
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

        KeyEventContext new_ev;
        new_ev.sender = nullptr;
        new_ev.key = event.key;
        new_ev.modifiers = event.modifiers;
        new_ev.keysym = event.keysym;
        new_ev.text = event.text;

        // We no longer update m_modifiers here; on_modifiers_event is the sole source of truth
        target->when_key_press.run(new_ev);

        // Global shortcuts
        if (event.key == KEY_F11)
        {
            if (this->is_fullscreen())
                this->unfullscreen();
            else
                this->fullscreen();
        }
        else if (event.key == KEY_Q && (m_modifiers & CTRL))
        {
            this->on_close();
        }
    }

    void WaylandWindow::handle_key_release(const KeyEvent &event)
    {
        if (!m_root)
            return;

        if (event.key == m_repeat_key)
        {
            m_is_repeating = false;
            m_repeat_key = 0;
        }

        Widget *target = m_focused ? m_focused : m_root.get();

        KeyEventContext new_ev;
        new_ev.sender = nullptr;
        new_ev.key = event.key;
        new_ev.modifiers = event.modifiers;
        new_ev.keysym = event.keysym;
        new_ev.text = event.text;
        target->when_key_release.run(new_ev);
    }

    void WaylandWindow::handle_move(const PointerEvent &event)
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
                    EventContext leave_ev;
                    leave_ev.sender = temp;
                    temp->when_mouse_leave.run(leave_ev);
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
                        EventContext leave_ev;
                        leave_ev.sender = w;
                        w->when_mouse_leave.run(leave_ev);
                    }
                }

                // Send MouseEnter to widgets in new path that were NOT in old path
                for (auto it = new_path.rbegin(); it != new_path.rend(); ++it)
                {
                    Widget *w = *it;
                    if (std::find(old_path.begin(), old_path.end(), w) == old_path.end())
                    {
                        EventContext enter_ev;
                        enter_ev.sender = w;
                        w->when_mouse_enter.run(enter_ev);
                    }
                }

                m_hovered = under;
            }
        }

        if (m_pressed)
        {
            MouseMoveEventContext new_ev;
            new_ev.sender = m_pressed;
            new_ev.x = (double)event.x;
            new_ev.y = (double)event.y;
            new_ev.modifiers = m_modifiers;

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
            MouseMoveEventContext new_ev;
            new_ev.sender = m_hovered;
            new_ev.x = (double)event.x;
            new_ev.y = (double)event.y;
            new_ev.modifiers = m_modifiers;

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

    void WaylandWindow::handle_press(const PointerEvent &event)
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

            MouseButtonEventContext new_ev;
            new_ev.sender = m_pressed;
            new_ev.button = event.button;
            new_ev.modifiers = m_modifiers;
            new_ev.serial = event.serial;
            new_ev.x = (double)event.x;
            new_ev.y = (double)event.y;

            // Collect parent chain first for safety (if a handler destroys the widget)
            std::vector<Widget *> chain;
            Widget *temp = m_pressed;
            while (temp)
            {
                chain.push_back(temp);
                temp = temp->parent();
            }

            for (Widget *w : chain)
            {
                // Verify widget is still registered (optional but safer)
                // If it was destroyed, it should have cleared m_pressed if it was m_pressed.
                // However, intermediate parents might not be easily verifiable here without
                // more complex tracking. The pre-collection already solves the invalid 'parent()'
                // call.
                new_ev.sender = w;
                w->when_mouse_press.run(new_ev);
                if (new_ev.stop_propagation)
                    break;
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

    void WaylandWindow::handle_release(const PointerEvent &event)
    {
        if (!m_pressed)
            return;

        MouseButtonEventContext new_ev;
        new_ev.sender = m_pressed;
        new_ev.button = event.button;
        new_ev.modifiers = m_modifiers;
        new_ev.serial = event.serial;
        new_ev.x = (double)event.x;
        new_ev.y = (double)event.y;

        // Collect parent chain first for safety (if a handler destroys the widget)
        std::vector<Widget *> chain;
        Widget *temp = m_pressed;
        while (temp)
        {
            chain.push_back(temp);
            temp = temp->parent();
        }

        for (Widget *w : chain)
        {
            new_ev.sender = w;
            w->when_mouse_release.run(new_ev);
            if (new_ev.stop_propagation)
                break;
        }
        m_pressed = nullptr;
    }

    void WaylandWindow::handle_wheel(const PointerEvent &event)
    {
        if (!m_root)
            return;

        Widget *under = m_root->hit_test(event.x, event.y);
        if (!under)
            return;

        MouseWheelEventContext new_ev;
        new_ev.sender = under;
        new_ev.dx = event.dx;
        new_ev.dy = event.dy;
        new_ev.x = event.x;
        new_ev.y = event.y;
        new_ev.modifiers = m_modifiers;

        Widget *temp = under;
        while (temp)
        {
            new_ev.sender = temp;
            temp->when_mouse_wheel.run(new_ev);
            if (new_ev.stop_propagation)
                break;
            temp = temp->parent();
        }
    }

    void WaylandWindow::set_blur(bool enabled)
    {
        if (m_compositor_context)
        {
            m_compositor_context->set_blur(enabled);
        }
    }

    bool WaylandWindow::is_fullscreen() const
    {
        return m_surface && m_surface->is_fullscreen();
    }

    void WaylandWindow::fullscreen()
    {
        if (m_compositor_context)
        {
            m_compositor_context->fullscreen();
            invalidate();
        }
    }

    void WaylandWindow::unfullscreen()
    {
        if (m_compositor_context)
        {
            m_compositor_context->unfullscreen();
            invalidate();
        }
    }

    bool WaylandWindow::was_maximized_before_minimize() const
    {
        return m_was_maximized_before_minimize;
    }

    bool WaylandWindow::is_minimized() const
    {
        return m_is_minimized;
    }

    WaylandSurface *WaylandWindow::w_surface() const
    {
        return m_surface.get();
    }

    static GLuint compile_shader(GLenum type, const char *source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            char info[512];
            glGetShaderInfoLog(shader, 512, nullptr, info);
            LOG_ERROR << "Shader compilation failed: " << info;
        }
        return shader;
    }

    void WaylandWindow::init_gl_resources()
    {
        if (m_gl_program)
            return;

        GLuint vshader = compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER);
        GLuint fshader = compile_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
        m_gl_program = glCreateProgram();
        glAttachShader(m_gl_program, vshader);
        glAttachShader(m_gl_program, fshader);
        glLinkProgram(m_gl_program);

        glGenBuffers(1, &m_gl_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_gl_vbo);
        float vertices[] = {
            -1.0f, 1.0f,  0.0f, 0.0f, 0.0f, // TL
            -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // BL
            1.0f,  1.0f,  0.0f, 1.0f, 0.0f, // TR
            1.0f,  -1.0f, 0.0f, 1.0f, 1.0f  // BR
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenTextures(1, &m_gl_texture);
        glBindTexture(GL_TEXTURE_2D, m_gl_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void WaylandWindow::render_gl_ui()
    {
        if (!m_surface || !m_surface->data())
            return;

        init_gl_resources();

        glViewport(0, 0, m_surface->width(), m_surface->height());
        if (is_transparent_surface())
            glClearColor(0, 0, 0, 0);
        else
            glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Premultiplied alpha (Cairo format)

        glUseProgram(m_gl_program);

        float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        GLint mvp_loc = glGetUniformLocation(m_gl_program, "u_mvp");
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, identity);

        GLint opacity_loc = glGetUniformLocation(m_gl_program, "u_opacity");
        glUniform1f(opacity_loc, 1.0f);

        // Upload Cairo buffer to texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_gl_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_surface->width(), m_surface->height(), 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, m_surface->data());

        GLint pos_attr = glGetAttribLocation(m_gl_program, "position");
        GLint tex_attr = glGetAttribLocation(m_gl_program, "texcoord");

        glBindBuffer(GL_ARRAY_BUFFER, m_gl_vbo);
        glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        glEnableVertexAttribArray(pos_attr);
        glVertexAttribPointer(tex_attr, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(tex_attr);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Background

        // Execute queued 3D draws
        for (const auto &draw : m_gl_queue)
        {
            if (draw.use_scissor)
            {
                glEnable(GL_SCISSOR_TEST);
                // Convert top-left (UI) to bottom-left (OpenGL)
                int gl_y = m_surface->height() - (draw.scissor_y + draw.scissor_h);
                glScissor(draw.scissor_x, gl_y, draw.scissor_w, draw.scissor_h);
            }
            else
            {
                glDisable(GL_SCISSOR_TEST);
            }

            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, draw.mvp);
            glUniform1f(opacity_loc, draw.opacity);
            glBindTexture(GL_TEXTURE_2D, draw.texture_id);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (draw.delete_texture)
            {
                glDeleteTextures(1, &draw.texture_id);
            }
        }
        glDisable(GL_SCISSOR_TEST);
        m_gl_queue.clear();

        m_surface->swap_buffers();
    }

    GraphicsContext &WaylandWindow::get_graphics_context() const
    {
        int w = width();
        int h = height();
        void *data = m_surface->data();

        if (!m_gc || !data || m_gc->width() != w || m_gc->height() != h)
        {
            LOG_INFO << "[APP] Creating new CairoGraphicsContext (" << w << "x" << h << ")";
            m_gc = std::make_unique<CairoGraphicContext>(this, data, w, h);
        }
        return *m_gc;
    }

    void WaylandWindow::queue_gl_draw(const GLDrawCall &call) const
    {
        m_gl_queue.push_back(call);
    }

    size_t WaylandWindow::add_timer(int ms, std::function<void()> callback, bool repeat)
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

    void WaylandWindow::stop_timer(size_t id)
    {
        m_timers.erase(id);
    }

    int WaylandWindow::width() const
    {
        return m_surface ? m_surface->width() : 0;
    }

    int WaylandWindow::height() const
    {
        return m_surface ? m_surface->height() : 0;
    }

    void WaylandWindow::unregister_widget(Widget *widget)
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

    void WaylandWindow::set_root(std::unique_ptr<Widget> root)
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

    void WaylandWindow::wakeup()
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

    void WaylandWindow::invalidate(Widget *widget)
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

    void WaylandWindow::quit()
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

    void WaylandWindow::post_task(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_task_queue.push_back(std::move(task));
        }
        wakeup();
    }

    void WaylandWindow::request_move()
    {
        if (m_compositor_context)
        {
            m_compositor_context->request_move(m_last_serial);
        }
    }

    void WaylandWindow::notify_app_manager(const std::string &type)
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

                    IpcClient client("/tmp/horizon_session.sock");
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

    void WaylandWindow::notify_window_state(bool minimized)
    {
        m_is_minimized = minimized;
        notify_app_manager("window_state_changed");
    }

    void WaylandWindow::maximize()
    {
        if (m_compositor_context)
        {
            m_compositor_context->maximize();
            m_is_minimized = false;
            notify_app_manager("app_started"); // Notify state change
            invalidate();                      // Ensure we repaint and commit a new buffer
            for (auto const &[id, handler] : m_on_maximize_handlers)
            {
                if (handler)
                    handler(true);
            }
        }
    }

    void WaylandWindow::minimize()
    {
        if (m_compositor_context)
        {
            m_was_maximized_before_minimize = is_maximized();
            m_compositor_context->minimize();
            notify_window_state(true);
            for (auto const &[id, handler] : m_on_minimize_handlers)
            {
                if (handler)
                    handler();
            }
        }
    }

    void WaylandWindow::restore(const std::string &token)
    {
        if (m_compositor_context)
        {
            m_compositor_context->restore(token);
            notify_window_state(false);
            invalidate(); // Ensure we repaint and commit a new buffer

            for (auto const &[id, handler] : m_on_maximize_handlers)
            {
                if (handler)
                    handler(m_was_maximized_before_minimize);
            }
        }
    }

    bool WaylandWindow::is_maximized() const
    {
        return m_surface && m_surface->is_maximized();
    }

}; // namespace horizon