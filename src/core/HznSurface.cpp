#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/IpcClient.hpp"
#include "horizon/LabwcCompositorContext.hpp"
#include "horizon/WayfireCompositorContext.hpp"
#include <GLES2/gl2.h>
#include <algorithm>
#include <glib-object.h>
#include <horizon/HznSurface.hpp>
#include <horizon/Logger.hpp>
#include <memory>
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

    HznSurface::HznSurface(std::string app_id, int w, int h, bool defer_init) : m_app_id(app_id)
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
    };

    HznSurface::~HznSurface()
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
    }

    void HznSurface::set_blur(bool enabled)
    {
        if (m_compositor_context)
        {
            m_compositor_context->set_blur(enabled);
        }
    }

    bool HznSurface::is_fullscreen() const
    {
        return m_surface && m_surface->is_fullscreen();
    }

    void HznSurface::fullscreen()
    {
        if (m_compositor_context)
        {
            m_compositor_context->fullscreen();
            invalidate();
        }
    }

    void HznSurface::unfullscreen()
    {
        if (m_compositor_context)
        {
            m_compositor_context->unfullscreen();
            invalidate();
        }
    }

    bool HznSurface::was_maximized_before_minimize() const
    {
        return m_was_maximized_before_minimize;
    }

    bool HznSurface::is_minimized() const
    {
        return m_is_minimized;
    }

    WaylandSurface *HznSurface::w_surface() const
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

    void HznSurface::init_gl_resources()
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

    void HznSurface::render_gl_ui()
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

    GraphicsContext &HznSurface::get_graphics_context() const
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

    void HznSurface::queue_gl_draw(const GLDrawCall &call) const
    {
        m_gl_queue.push_back(call);
    }

    size_t HznSurface::add_timer(int ms, std::function<void()> callback, bool repeat)
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

    void HznSurface::stop_timer(size_t id)
    {
        m_timers.erase(id);
    }

    int HznSurface::width() const
    {
        return m_surface ? m_surface->width() : 0;
    }

    int HznSurface::height() const
    {
        return m_surface ? m_surface->height() : 0;
    }

    void HznSurface::unregister_widget(Widget *widget)
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

    void HznSurface::set_root(std::unique_ptr<Widget> root)
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

    void HznSurface::wakeup()
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

    void HznSurface::invalidate(Widget *widget)
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

    void HznSurface::quit()
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

    void HznSurface::post_task(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(m_task_mutex);
            m_task_queue.push_back(std::move(task));
        }
        wakeup();
    }

    void HznSurface::request_move()
    {
        if (m_compositor_context)
        {
            m_compositor_context->request_move(m_last_serial);
        }
    }

    void HznSurface::notify_app_manager(const std::string &type)
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

    void HznSurface::notify_window_state(bool minimized)
    {
        m_is_minimized = minimized;
        notify_app_manager("window_state_changed");
    }

    void HznSurface::maximize()
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

    void HznSurface::minimize()
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

    void HznSurface::restore(const std::string &token)
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

    bool HznSurface::is_maximized() const
    {
        return m_surface && m_surface->is_maximized();
    }

}; // namespace horizon