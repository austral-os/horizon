#include <horizon/Application.hpp>
#include <horizon/WaylandSurface.hpp>
#include <horizon/Menu.hpp>
#include <horizon/Window.hpp>
#include <horizon/Widget.hpp>
#include <horizon/CairoGraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/IpcClient.hpp>
#include <nlohmann/json.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <horizon/xdg-activation-v1-client-protocol.h>
#include <wayland-client.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include <fontconfig/fontconfig.h>
#include <glib.h>

namespace horizon
{
    // Ensure this matches the friend declaration in the header
    void registry_global(void *data, struct ::wl_registry *registry, uint32_t id, const char *interface,
                         uint32_t version)
    {
        Application *app = static_cast<Application *>(data);
        if (strcmp(interface, "wl_compositor") == 0) {
            app->m_compositor = static_cast<struct ::wl_compositor *>(wl_registry_bind(registry, id, &wl_compositor_interface, 4));
        } else if (strcmp(interface, "wl_shm") == 0) {
            app->m_shm = static_cast<struct ::wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
        } else if (strcmp(interface, "xdg_wm_base") == 0) {
            app->m_xdg_wm_base = static_cast<struct ::xdg_wm_base *>(wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
            static const xdg_wm_base_listener wm_list = { .ping = [](void *, xdg_wm_base *wm, uint32_t ser) { xdg_wm_base_pong(wm, ser); }};
            xdg_wm_base_add_listener(app->m_xdg_wm_base, &wm_list, nullptr);
        } else if (strcmp(interface, "wl_seat") == 0) {
            app->m_seat = static_cast<struct ::wl_seat *>(wl_registry_bind(registry, id, &wl_seat_interface, 1));
            static const struct wl_seat_listener s_list = {
                .capabilities = [](void *data, struct wl_seat *seat, uint32_t caps) {
                    Application *app = static_cast<Application *>(data);
                    if (caps & WL_SEAT_CAPABILITY_POINTER) {
                        struct wl_pointer *pointer = wl_seat_get_pointer(seat);
                        static const struct wl_pointer_listener p_list = {
                            .enter = [](void *d, struct wl_pointer *, uint32_t ser, struct wl_surface *s, wl_fixed_t sx, wl_fixed_t sy) {
                                Application *a = static_cast<Application *>(d);
                                a->m_last_serial = ser;
                                a->m_pointer_x = wl_fixed_to_double(sx);
                                a->m_pointer_y = wl_fixed_to_double(sy);
                                if (a->m_window_map.count(s)) {
                                    a->m_pointer_window = a->m_window_map[s];
                                    LOG_INFO << "[INPUT] Pointer window set to: " << a->m_pointer_window->title();
                                    PointerEvent ev{PointerEvent::Type::Enter, a->m_pointer_x, a->m_pointer_y, 0, ser};
                                    a->on_pointer_event(ev);
                                }
                            },
                            .leave = [](void *d, struct wl_pointer *, uint32_t ser, struct wl_surface *s) {
                                Application *a = static_cast<Application *>(d);
                                a->m_last_serial = ser;
                                if (a->m_pointer_window) {
                                    PointerEvent ev{PointerEvent::Type::Leave, a->m_pointer_x, a->m_pointer_y, 0, ser};
                                    a->on_pointer_event(ev);
                                    a->m_pointer_window = nullptr;
                                }
                            },
                            .motion = [](void *d, struct wl_pointer *, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
                                Application *a = static_cast<Application *>(d);
                                a->m_pointer_x = wl_fixed_to_double(sx);
                                a->m_pointer_y = wl_fixed_to_double(sy);
                                PointerEvent ev{PointerEvent::Type::Move, a->m_pointer_x, a->m_pointer_y};
                                a->on_pointer_event(ev);
                            },
                            .button = [](void *d, struct wl_pointer *, uint32_t ser, uint32_t time, uint32_t button, uint32_t state) {
                                Application *a = static_cast<Application *>(d);
                                a->m_last_serial = ser;
                                PointerEvent ev{(state == WL_POINTER_BUTTON_STATE_PRESSED) ? PointerEvent::Type::Press : PointerEvent::Type::Release, 
                                                a->m_pointer_x, a->m_pointer_y, button, ser};
                                a->on_pointer_event(ev);
                            },
                            .axis = [](void *d, struct wl_pointer *, uint32_t time, uint32_t axis, wl_fixed_t val) {
                                Application *a = static_cast<Application *>(d);
                                PointerEvent ev{PointerEvent::Type::Scroll, a->m_pointer_x, a->m_pointer_y};
                                if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) ev.dy = wl_fixed_to_double(val);
                                else ev.dx = wl_fixed_to_double(val);
                                a->on_pointer_event(ev);
                            }
                        };
                        wl_pointer_add_listener(pointer, &p_list, app);
                    }
                    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
                        struct wl_keyboard *keyboard = wl_seat_get_keyboard(seat);
                        static const wl_keyboard_listener k_list = {
                            .keymap = [](void *d, struct wl_keyboard *, uint32_t format, int32_t fd, uint32_t size) {
                                Application *a = static_cast<Application *>(d);
                                if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) { LOG_ERROR << "Unsupported keymap format"; close(fd); return; }
                                char *map_str = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
                                if (map_str == MAP_FAILED) { close(fd); return; }
                                if (a->m_xkb_keymap) xkb_keymap_unref(a->m_xkb_keymap);
                                a->m_xkb_keymap = xkb_keymap_new_from_string(a->m_xkb_context, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
                                munmap(map_str, size); close(fd);
                                if (a->m_xkb_state) xkb_state_unref(a->m_xkb_state);
                                a->m_xkb_state = xkb_state_new(a->m_xkb_keymap);
                            },
                            .enter = [](void *d, struct wl_keyboard *, uint32_t ser, struct wl_surface *s, struct wl_array *) {
                                Application *a = static_cast<Application *>(d);
                                a->m_last_serial = ser;
                                if (a->m_window_map.count(s)) {
                                    a->m_keyboard_window = a->m_window_map[s];
                                    a->on_activated(true);
                                }
                            },
                            .leave = [](void *d, struct wl_keyboard *, uint32_t ser, struct wl_surface *s) {
                                Application *a = static_cast<Application *>(d);
                                a->m_last_serial = ser;
                                a->on_activated(false);
                                a->m_keyboard_window = nullptr;
                            },
                            .key = [](void *d, struct wl_keyboard *, uint32_t ser, uint32_t time, uint32_t key, uint32_t state) {
                                Application *a = static_cast<Application *>(d);
                                a->m_last_serial = ser;
                                KeyEvent ev{(state == WL_KEYBOARD_KEY_STATE_PRESSED) ? KeyEvent::Type::Press : KeyEvent::Type::Release, key, 0, 0, ser};
                                if (a->m_xkb_state) {
                                    xkb_keysym_t sym = xkb_state_key_get_one_sym(a->m_xkb_state, key + 8);
                                    ev.keysym = sym;
                                    char buf[64]; xkb_keysym_to_utf8(sym, buf, sizeof(buf)); ev.text = buf;
                                    auto check_mod = [&](const char* name, Application::Modifier m) {
                                        if (xkb_state_mod_name_is_active(a->m_xkb_state, name, XKB_STATE_MODS_EFFECTIVE)) ev.modifiers |= m;
                                    };
                                    check_mod(XKB_MOD_NAME_SHIFT, Application::Modifier::SHIFT);
                                    check_mod(XKB_MOD_NAME_CTRL, Application::Modifier::CTRL);
                                    check_mod(XKB_MOD_NAME_ALT, Application::Modifier::ALT);
                                    check_mod(XKB_MOD_NAME_LOGO, Application::Modifier::LOGO);
                                }
                                a->on_key_event(ev);
                            },
                            .modifiers = [](void *d, struct wl_keyboard *, uint32_t ser, uint32_t dep, uint32_t lat, uint32_t lck, uint32_t group) {
                                Application *a = static_cast<Application *>(d);
                                if (a->m_xkb_state) {
                                    xkb_state_update_mask(a->m_xkb_state, dep, lat, lck, 0, 0, group);
                                    uint32_t mods = 0;
                                    auto check_mod = [&](const char* name, Application::Modifier m) {
                                        if (xkb_state_mod_name_is_active(a->m_xkb_state, name, XKB_STATE_MODS_EFFECTIVE)) mods |= m;
                                    };
                                    check_mod(XKB_MOD_NAME_SHIFT, Application::Modifier::SHIFT);
                                    check_mod(XKB_MOD_NAME_CTRL, Application::Modifier::CTRL);
                                    check_mod(XKB_MOD_NAME_ALT, Application::Modifier::ALT);
                                    check_mod(XKB_MOD_NAME_LOGO, Application::Modifier::LOGO);
                                    a->on_modifiers_event(mods);
                                }
                            },
                            .repeat_info = [](void *, struct wl_keyboard *, int32_t, int32_t) {}
                        };
                        wl_keyboard_add_listener(keyboard, &k_list, app);
                    }
                },
                .name = [](void *, struct wl_seat *, const char *) {}
            };
            wl_seat_add_listener(app->m_seat, &s_list, app);
        }
    }

    static void registry_global_remove(void *, struct ::wl_registry *, uint32_t) {}

    Application::Application(const std::string &app_id, int w, int h, bool is_service)
        : m_app_id(app_id), m_width(w), m_height(h), m_is_service(is_service)
    {
        g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);
        FcInit();

        signal(SIGPIPE, SIG_IGN);
        m_wakeup_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        
        Logger::instance().init(app_id);
        LOG_INFO << "Application initializing: " << app_id;

        m_display = wl_display_connect(nullptr);
        if (!m_display) throw std::runtime_error("Failed to connect to Wayland");
 
        m_registry = wl_display_get_registry(m_display);
        static const wl_registry_listener listener = {
             [](void *data, struct wl_registry *r, uint32_t id, const char *interface, uint32_t v) {
                 registry_global(data, r, id, interface, v);
             },
             registry_global_remove
        };
        wl_registry_add_listener(m_registry, &listener, this);
        wl_display_roundtrip(m_display);

        init_egl();
        
        m_xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        theme_manager = std::make_unique<ThemeManager>();
    }

    void Application::invalidate(Widget *w)
    {
        if (w)
        {
            // If we had a widget-to-window map we could be more specific,
            // but for now let's just invalidate all windows if anything changes
            // or we can just do nothing if we assume widgets invalidate themselves.
            for (auto &window : m_windows)
            {
                window->invalidate();
            }
        }
        else
        {
            for (auto &window : m_windows)
            {
                window->invalidate();
            }
        }
    }
    void Application::init_egl()
    {
        m_egl_display = eglGetDisplay((EGLNativeDisplayType)m_display);
        if (m_egl_display == EGL_NO_DISPLAY) {
            LOG_ERROR << "eglGetDisplay failed: 0x" << std::hex << eglGetError();
            return;
        }

        if (!eglInitialize(m_egl_display, nullptr, nullptr)) {
            LOG_ERROR << "eglInitialize failed: 0x" << std::hex << eglGetError();
            return;
        }

        eglBindAPI(EGL_OPENGL_ES_API);

        EGLint attr[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                        EGL_ALPHA_SIZE, 8, EGL_BLUE_SIZE, 8,
                        EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                        EGL_NONE};
        EGLint num_configs;
        if (!eglChooseConfig(m_egl_display, attr, &m_egl_config, 1, &num_configs) || num_configs == 0) {
            LOG_ERROR << "eglChooseConfig failed: 0x" << std::hex << eglGetError();
            return;
        }

        EGLint ctx_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        m_egl_context = eglCreateContext(m_egl_display, m_egl_config, EGL_NO_CONTEXT, ctx_attr);
        if (m_egl_context == EGL_NO_CONTEXT) {
            LOG_ERROR << "eglCreateContext failed: 0x" << std::hex << eglGetError();
            return;
        }
        
        // Initialize GLES2 Resources
        if (!eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_egl_context)) {
            LOG_ERROR << "eglMakeCurrent (surfaceless) failed: 0x" << std::hex << eglGetError();
        }
        
        const char *v_shader_src = 
            "attribute vec2 position;\n"
            "attribute vec2 texcoord;\n"
            "varying vec2 v_texcoord;\n"
            "uniform mat4 mvp;\n"
            "void main() {\n"
            "    v_texcoord = texcoord;\n"
            "    gl_Position = mvp * vec4(position, 0.0, 1.0);\n"
            "}\n";

        const char *f_shader_src =
            "precision mediump float;\n"
            "varying vec2 v_texcoord;\n"
            "uniform sampler2D tex;\n"
            "uniform float opacity;\n"
            "void main() {\n"
            "    vec4 color = texture2D(tex, v_texcoord);\n"
            "    gl_FragColor = vec4(color.b, color.g, color.r, color.a) * opacity;\n"
            "}\n";

        m_v_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(m_v_shader, 1, &v_shader_src, nullptr);
        glCompileShader(m_v_shader);
        
        GLint compiled;
        glGetShaderiv(m_v_shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char info[512];
            glGetShaderInfoLog(m_v_shader, 512, nullptr, info);
            LOG_ERROR << "Vertex shader compile error: " << info;
        }

        m_f_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(m_f_shader, 1, &f_shader_src, nullptr);
        glCompileShader(m_f_shader);
        glGetShaderiv(m_f_shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char info[512];
            glGetShaderInfoLog(m_f_shader, 512, nullptr, info);
            LOG_ERROR << "Fragment shader compile error: " << info;
        }

        m_program = glCreateProgram();
        glAttachShader(m_program, m_v_shader);
        glAttachShader(m_program, m_f_shader);
        glLinkProgram(m_program);
        
        GLint linked;
        glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char info[512];
            glGetProgramInfoLog(m_program, 512, nullptr, info);
            LOG_ERROR << "Program link error: " << info;
        }

        m_pos_loc = glGetAttribLocation(m_program, "position");
        m_uv_loc = glGetAttribLocation(m_program, "texcoord");
        m_mvp_loc = glGetUniformLocation(m_program, "mvp");
        m_tex_loc = glGetUniformLocation(m_program, "tex");
        m_opacity_loc = glGetUniformLocation(m_program, "opacity");

        glGenBuffers(1, &m_vbo);
        
        eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    void Application::register_window(Window* window) {
        if (window && window->w_surface() && window->w_surface()->surface()) {
            m_window_map[window->w_surface()->surface()] = window;
        }
    }

    void Application::unregister_window(Window* window) {
        if (!window) return;
        LOG_INFO << "Unregistering window: " << window->title() << " (Remaining: " << m_windows.size() << ")";
        if (window->w_surface() && window->w_surface()->surface()) {
            m_window_map.erase(window->w_surface()->surface());
        }
        auto it = std::find_if(m_windows.begin(), m_windows.end(), [window](const std::unique_ptr<Window>& p) { return p.get() == window; });
        if (it != m_windows.end()) m_windows.erase(it);
        LOG_INFO << "Window removed from list. Remaining: " << m_windows.size();
        if (m_windows.empty()) {
            LOG_INFO << "No more windows left. Quitting application.";
            quit();
        }
    }

    void Application::set_root_window(std::unique_ptr<Window> window) {
        register_window(window.get());
        m_windows.push_back(std::move(window));
    }

    void Application::set_root(std::unique_ptr<Window> window) {
        set_root_window(std::move(window));
    }
 
    void Application::run() {
        if (!m_display) {
            LOG_ERROR << "Cannot run architecture without wayland display";
            return;
        }
        LOG_INFO << "Entering main loop with " << m_windows.size() << " windows";
        m_is_running = true;
        int iter = 0;
        while (m_is_running) {
            iter++;
            if (iter < 10) LOG_INFO << "Loop iteration " << iter;
            dispatch_events();
            render_gl_ui(iter);
            
            // Wayland Loop Step 1: Dispatch any pending events already in our local queue
            while (wl_display_prepare_read(m_display) != 0) {
                wl_display_dispatch_pending(m_display);
            }

            // Calculate next timeout based on timers
            int timeout = -1;
            {
                std::lock_guard<std::recursive_mutex> lock(m_timer_mutex);
                auto now = std::chrono::steady_clock::now();
                
                // Collect callbacks to run outside the lock
                std::vector<std::function<void()>> callbacks_to_run;

                for (auto it = m_timers.begin(); it != m_timers.end(); ) {
                    if (now >= it->next_run) {
                        auto& t = *it;
                        callbacks_to_run.push_back(t.callback);
                        if (t.repeat) {
                            t.next_run = now + std::chrono::milliseconds(t.interval);
                            ++it;
                        } else {
                            it = m_timers.erase(it);
                        }
                    } else {
                        ++it;
                    }
                }

                if (!m_timers.empty()) {
                    auto next_timer = std::min_element(m_timers.begin(), m_timers.end(),
                        [](const Timer& a, const Timer& b) { return a.next_run < b.next_run; });
                    auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
                        next_timer->next_run - std::chrono::steady_clock::now()).count();
                    timeout = std::max(0, (int)delay);
                }

                // Run callbacks outside the lock to avoid deadlocks
                for (auto& cb : callbacks_to_run) {
                    cb();
                }
            }

            // Wayland + Eventfd poll
            struct pollfd fds[2];
            fds[0].fd = wl_display_get_fd(m_display);
            fds[0].events = POLLIN;
            fds[1].fd = m_wakeup_fd;
            fds[1].events = POLLIN;

            wl_display_flush(m_display);
            int poll_res = poll(fds, 2, timeout);

            if (poll_res > 0) {
                if (fds[0].revents & POLLIN) {
                    if (wl_display_read_events(m_display) == -1) {
                         LOG_ERROR << "wl_display_read_events failed";
                         m_is_running = false;
                    }
                } else {
                    wl_display_cancel_read(m_display);
                }
                
                if (fds[1].revents & POLLIN) {
                    uint64_t v;
                    if (read(m_wakeup_fd, &v, 8) < 0) { /* Non-blocking */ }
                }
            } else {
                wl_display_cancel_read(m_display);
            }
            
            wl_display_dispatch_pending(m_display);
        }
        LOG_INFO << "Main loop exited. m_is_running=" << m_is_running;
    }

    void Application::quit() { m_is_running = false; wakeup(); }
    void Application::wakeup() { uint64_t v = 1; write(m_wakeup_fd, &v, 8); }
    void Application::post_task(std::function<void()> t) { std::lock_guard<std::recursive_mutex> l(m_task_mutex); m_task_queue.push_back(t); wakeup(); }
    void Application::dispatch_events() {
        std::deque<std::function<void()>> ts;
        {
            std::lock_guard<std::recursive_mutex> l(m_task_mutex);
            ts.swap(m_task_queue);
        }
        for (auto& t : ts) t();
    }

    size_t Application::add_timer(int interval_ms, std::function<void()> callback, bool repeat) {
        std::lock_guard<std::recursive_mutex> lock(m_timer_mutex);
        Timer t;
        t.id = m_next_timer_id++;
        t.interval = interval_ms;
        t.callback = callback;
        t.repeat = repeat;
        t.next_run = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms);
        m_timers.push_back(t);
        wakeup();
        return t.id;
    }

    void Application::stop_timer(size_t timer_id) {
        std::lock_guard<std::recursive_mutex> lock(m_timer_mutex);
        m_timers.erase(std::remove_if(m_timers.begin(), m_timers.end(),
            [timer_id](const Timer& t) { return t.id == timer_id; }), m_timers.end());
    }

    void Application::on_pointer_event(const PointerEvent &e) { if (m_pointer_window) m_pointer_window->on_pointer_event(e); }
    void Application::on_key_event(const KeyEvent &e) { 
        if (m_keyboard_window) m_keyboard_window->on_key_event(e); 
    }
    void Application::on_modifiers_event(uint32_t m) { if (m_keyboard_window) m_keyboard_window->on_modifiers_event(m); }
    void Application::on_resize(int w, int h) {}
    void Application::on_activated(bool a) {}
    void Application::on_foreign_toplevel_event() {}
 
    void Application::send_remote_signal(pid_t target_pid, const std::string &signal_name, const std::string &data)
    {
        nlohmann::json msg;
        msg["type"] = "remote_signal";
        msg["target_pid"] = target_pid;
        msg["signal"] = signal_name;
        if (!data.empty())
        {
            msg["data"] = data;
        }
        msg["sender_pid"] = getpid();
 
        IpcClient client("/tmp/horizon_session.sock");
        client.send(msg.dump());
    }
 
    void Application::set_global_menu(const std::vector<Menu *> &menus)
    {
        nlohmann::json msg;
        msg["type"] = "set_global_menu";
        msg["pid"] = getpid();
        
        nlohmann::json menus_json = nlohmann::json::array();
        for (auto* m : menus) {
            nlohmann::json mj;
            mj["title"] = m->title();
            mj["bold"] = m->bold();
            mj["icon"] = m->icon_name();
            // In a real implementation, we'd serialize items here
             menus_json.push_back(mj);
        }
        msg["menus"] = menus_json;

        IpcClient client("/tmp/horizon_session.sock");
        client.send(msg.dump());
    }

    void Application::render_gl_ui(int iter)
    {
        if (m_windows.empty()) return;

        for (auto &window : m_windows)
        {
            if (!window->w_surface()) {
                 static int skip_count = 0;
                 if (skip_count++ < 5) LOG_INFO << "Skipping window " << window->title() << ": no surface";
                 continue;
            }
            if (!window->w_surface()->egl_surface()) {
                 static int skip_count_egl = 0;
                 if (skip_count_egl++ < 5) LOG_INFO << "Skipping window " << window->title() << ": no EGL surface";
                 continue;
            }
            if (!window->w_surface()->is_configured()) {
                 static int skip_count_conf = 0;
                 if (skip_count_conf++ < 5) LOG_INFO << "Skipping window " << window->title() << ": not configured yet";
                 continue;
            }

            EGLSurface surface = (EGLSurface)window->w_surface()->egl_surface();
            if (iter < 5) LOG_INFO << "Rendering window " << window->title() << " size: " << window->width() << "x" << window->height();

            // Only redraw if dirty
            bool is_dirty = window->m_full_repaint || !window->m_dirty_widgets.empty() || !window->m_gl_queue.empty();
            if (!is_dirty) continue;

            if (!eglMakeCurrent(m_egl_display, surface, surface, m_egl_context)) {
                LOG_ERROR << "eglMakeCurrent failed for window " << window->title() << " error: 0x" << std::hex << eglGetError();
                continue;
            }

            // 1. Cairo Draw Phase
            // Use a local buffer for Cairo to avoid conflicts with mmap/SHM memory
            // which was causing munmap_chunk errors in FontConfig/Cairo.
            std::vector<unsigned char> cairo_buffer(window->width() * window->height() * 4, 0);

            CairoGraphicContext gc(this, window.get(), cairo_buffer.data(), window->width(), window->height());
            window->render(gc, 0, 0, window->width(), window->height(), window->m_full_repaint);
            gc.flush();

            // 2. OpenGL Phase
            glViewport(0, 0, window->width(), window->height());
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // Dark Gray
            glClear(GL_COLOR_BUFFER_BIT);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(m_program);
            
            // Upload Cairo buffer to main texture
            if (window->m_main_texture == 0) {
                glGenTextures(1, &window->m_main_texture);
                glBindTexture(GL_TEXTURE_2D, window->m_main_texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            
            glBindTexture(GL_TEXTURE_2D, window->m_main_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window->width(), window->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, cairo_buffer.data());

            // Redundant diagnostic check removed

            GLenum gl_err = glGetError();
            if (gl_err != GL_NO_ERROR) {
                LOG_ERROR << "GL Error before draw: " << gl_err;
            }

            // Base blit ...

            // Base blit (Cairo UI)
            glActiveTexture(GL_TEXTURE0);
            glUniform1i(m_tex_loc, 0);
            float base_mvp[16] = {
                2.0f/window->width(), 0, 0, 0,
                0, -2.0f/window->height(), 0, 0,
                0, 0, 1, 0,
                -1, 1, 0, 1
            };
            glUniformMatrix4fv(m_mvp_loc, 1, GL_FALSE, base_mvp);
            glUniform1f(m_opacity_loc, 1.0f);
            glDisable(GL_SCISSOR_TEST);

            float vertices[] = {
                0.0f, 0.0f, 0.0f, 0.0f,
                (float)window->width(), 0.0f, 1.0f, 0.0f,
                0.0f, (float)window->height(), 0.0f, 1.0f,
                (float)window->width(), (float)window->height(), 1.0f, 1.0f
            };

            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(m_pos_loc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glEnableVertexAttribArray(m_pos_loc);
            glVertexAttribPointer(m_uv_loc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(m_uv_loc);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Process GLDrawCall queue (for 3D widgets / complex effects)
            while (!window->m_gl_queue.empty())
            {
                auto call = window->m_gl_queue.front();
                window->m_gl_queue.pop_front();

                glBindTexture(GL_TEXTURE_2D, call.texture_id);
                glUniformMatrix4fv(m_mvp_loc, 1, GL_FALSE, call.mvp);
                glUniform1f(m_opacity_loc, call.opacity);

                if (call.use_scissor) {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(call.scissor_x, window->height() - call.scissor_y - call.scissor_h, call.scissor_w, call.scissor_h);
                } else {
                    glDisable(GL_SCISSOR_TEST);
                }

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                if (call.delete_texture) {
                    glDeleteTextures(1, &call.texture_id);
                }
            }

            if (!eglSwapBuffers(m_egl_display, surface)) {
                LOG_ERROR << "eglSwapBuffers failed: " << eglGetError();
            }
            eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
    }

    WaylandSurface* Application::w_surface() const
    {
        if (!m_windows.empty()) return m_windows[0]->w_surface();
        return nullptr;
    }
 
    void Application::on_close() {}

    Application::~Application() {
        if (m_wakeup_fd >= 0) close(m_wakeup_fd);
        
        // Destroy windows first (they'll cleanup their own surfaces)
        m_windows.clear();

        if (m_egl_display != EGL_NO_DISPLAY) {
            if (m_egl_context != EGL_NO_CONTEXT) {
                eglDestroyContext(m_egl_display, m_egl_context);
                m_egl_context = EGL_NO_CONTEXT;
            }
            eglTerminate(m_egl_display);
            m_egl_display = EGL_NO_DISPLAY;
        }

        if (m_xdg_wm_base) {
            xdg_wm_base_destroy(m_xdg_wm_base);
            m_xdg_wm_base = nullptr;
        }

        if (m_layer_shell) {
            zwlr_layer_shell_v1_destroy(m_layer_shell);
            m_layer_shell = nullptr;
        }

        if (m_activation) {
            xdg_activation_v1_destroy(m_activation);
            m_activation = nullptr;
        }

        if (m_compositor) {
            wl_compositor_destroy(m_compositor);
            m_compositor = nullptr;
        }

        if (m_shm) {
            wl_shm_destroy(m_shm);
            m_shm = nullptr;
        }

        if (m_registry) {
            wl_registry_destroy(m_registry);
            m_registry = nullptr;
        }

        if (m_display) {
            wl_display_disconnect(m_display);
            m_display = nullptr;
        }

        if (m_xkb_state) xkb_state_unref(m_xkb_state);
        if (m_xkb_keymap) xkb_keymap_unref(m_xkb_keymap);
        if (m_xkb_context) xkb_context_unref(m_xkb_context);
    }
}
