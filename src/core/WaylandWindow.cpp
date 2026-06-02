#include "MainThreadDataSink.hpp"
#include "WaylandClipboardBackend.hpp"
#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/ClientMenu.hpp"
#include "horizon/IpcClient.hpp"
#include "horizon/LabwcCompositorContext.hpp"
#include "horizon/Menu.hpp"
#include "horizon/Vault.hpp"
#include "horizon/WayfireCompositorContext.hpp"
#include "horizon/Window.hpp"
#include <horizon/dialogs/PrintDialog.hpp>
#include "horizon/SystemInfo.hpp"
#include "horizon/dialogs/AboutUsDialog.hpp"
#include <GLES2/gl2.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <glib-object.h>
#include <horizon/DialogTypes.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Notification.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/dialogs/DialogPreferences.hpp>
#include <horizon/dialogs/FileDialog.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <horizon/dialogs/MessageDialog.hpp>
#include <horizon/dialogs/PreferencesContent.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <linux/input-event-codes.h>
#include <memory>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace horizon
{
    WaylandWindow *WaylandWindow::m_active_window = nullptr;

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
        "uniform float u_gradient_start;\n"
        "uniform float u_gradient_end;\n"
        "uniform float u_gradient_horizontal;\n"
        "void main() {\n"
        "    float mix_val = mix(v_texcoord.y, v_texcoord.x, u_gradient_horizontal);\n"
        "    float gradient = mix(u_gradient_start, u_gradient_end, mix_val);\n"
        "    vec4 tex = texture2D(u_texture, v_texcoord);\n"
        "    gl_FragColor = vec4(tex.b, tex.g, tex.r, tex.a) * u_opacity * gradient;\n"
        "}\n";

    WaylandWindow::WaylandWindow(std::string app_id, int w, int h, bool defer_init, bool resizable,
                                 int min_w, int min_h)
        : m_app_id(app_id), m_resizable(resizable), m_min_width(min_w), m_min_height(min_h),
          m_popup_menu(nullptr), m_popup_vault(nullptr)
    {
        // Inicialización del sistema
        m_surface = std::make_unique<WaylandSurface>(w, h);
        if (!defer_init)
        {
            m_surface->init_display();
            m_surface->setup_xdg_toplevel(m_name, m_app_id);
            if (!m_resizable)
            {
                m_surface->set_min_size(w, h);
                m_surface->set_max_size(w, h);
            }
            else if (m_min_width > 0 || m_min_height > 0)
            {
                m_surface->set_min_size(std::max(0, m_min_width), std::max(0, m_min_height));
            }
            m_clipboard_backend = std::make_unique<WaylandClipboardBackend>(m_surface.get());
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

        theme_manager()->when_change.connect(
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
        m_active_window = this;
    };

    WaylandWindow::~WaylandWindow()
    {
        if (m_active_window == this)
        {
            m_active_window = nullptr;
        }

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

        // Cleanup GL resources
        if (m_gl_program)
        {
            glDeleteProgram(m_gl_program);
            m_gl_program = 0;
        }
        if (m_gl_vbo)
        {
            glDeleteBuffers(1, &m_gl_vbo);
            m_gl_vbo = 0;
        }
        if (m_gl_texture)
        {
            glDeleteTextures(1, &m_gl_texture);
            m_gl_texture = 0;
        }

        // WaylandSurface is managed by unique_ptr, it will be freed automatically
        // when its destructor is called after this body finishes.

        if (m_wakeup_fd >= 0)
        {
            close(m_wakeup_fd);
        }
    }

    void WaylandWindow::set_clipboard_data(const ClipboardData &data)
    {
        if (m_clipboard_backend)
        {
            m_clipboard_backend->set(data);
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
            if (m_client_menu && m_is_activated)
            {
                m_client_menu->set_global_menu(
                    {}); // Clear global menu before exit, only if we were active
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
            const auto &foreigns = m_surface->get_foreign_toplevels();
            for (const auto &pair : foreigns)
            {
                const auto &ft = pair.second;
                if (ft.app_id.empty() && ft.title.empty())
                    continue;

                ApplicationInfo info;
                info.app_id = ft.app_id;
                info.title = ft.title;
                info.handle = ft.handle;
                info.is_active = ft.active;
                info.is_minimized = ft.minimized;
                info.show_in_dock = true;
                ctx.apps.push_back(info);
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

        if (!m_resizable)
        {
            m_surface->set_min_size(m_surface->width(), m_surface->height());
            m_surface->set_max_size(m_surface->width(), m_surface->height());
        }
        else if (m_min_width > 0 || m_min_height > 0)
        {
            m_surface->set_min_size(std::max(0, m_min_width), std::max(0, m_min_height));
        }

        if (!m_clipboard_backend)
        {
            m_clipboard_backend = std::make_unique<WaylandClipboardBackend>(m_surface.get());
        }
    }

    void WaylandWindow::set_resizable(bool resizable)
    {
        m_resizable = resizable;
        if (m_surface)
        {
            if (!m_resizable)
            {
                m_surface->set_min_size(m_surface->width(), m_surface->height());
                m_surface->set_max_size(m_surface->width(), m_surface->height());
            }
            else
            {
                m_surface->set_min_size(std::max(0, m_min_width), std::max(0, m_min_height));
                m_surface->set_max_size(0, 0);
            }
        }
    }

    void WaylandWindow::set_min_size(int w, int h)
    {
        m_min_width = w;
        m_min_height = h;

        if (m_surface && m_resizable)
        {
            m_surface->set_min_size(std::max(0, m_min_width), std::max(0, m_min_height));
        }
    }

    void WaylandWindow::run()
    {
        LOG_INFO << "[WINDOW] Starting event loop for app_id: " << m_app_id;
        m_is_running = true;
        if (m_use_global_menu)
        {
            init_global_menu();
        }

        signal_manager.connect("preferences",
                               [this](SignalContext &)
                               {
                                   LOG_INFO << "WaylandWindow: Received 'preferences' signal";
                                   this->show_preferences();
                               });

        signal_manager.connect("aboutus",
                               [this](SignalContext &)
                               {
                                   LOG_INFO << "WaylandWindow: Received 'aboutus' signal";
                                   this->show_aboutus();
                               });

        // Standard clipboard signal routing: automatically dispatch to best candidate
        signal_manager.connect("copy",
                               [this](SignalContext &)
                               {
                                   LOG_INFO
                                       << "WaylandWindow: Received 'copy' signal from menu/IPC";
                                   auto *target = find_clipboard_target();
                                   if (target)
                                       target->perform(ClipboardAction::Copy);
                                   else
                                       LOG_INFO
                                           << "WaylandWindow: No clipboard target found for 'copy'";
                               });
        signal_manager.connect("cut",
                               [this](SignalContext &)
                               {
                                   LOG_INFO << "WaylandWindow: Received 'cut' signal from menu/IPC";
                                   auto *target = find_clipboard_target();
                                   if (target)
                                       target->perform(ClipboardAction::Cut);
                                   else
                                       LOG_INFO
                                           << "WaylandWindow: No clipboard target found for 'cut'";
                               });
        signal_manager.connect(
            "paste",
            [this](SignalContext &)
            {
                LOG_INFO << "WaylandWindow: Received 'paste' signal from menu/IPC";
                auto *target = find_clipboard_target();
                if (target)
                    target->perform(ClipboardAction::Paste);
                else
                    LOG_INFO << "WaylandWindow: No clipboard target found for 'paste'";
            });
        signal_manager.connect(
            "undo",
            [this](SignalContext &)
            {
                LOG_INFO << "WaylandWindow: Received 'undo' signal from menu/IPC";
                auto *target = find_undo_target();
                if (target)
                {
                    EventContext ctx;
                    ctx.sender = target;
                    target->when_undo.run(ctx);
                }
                else
                {
                    LOG_INFO << "WaylandWindow: No undo target found";
                }
            });
        signal_manager.connect(
            "print",
            [this](SignalContext &)
            {
                LOG_INFO << "WaylandWindow: Received 'print' signal from menu/IPC";
                auto *target = find_print_target();
                if (target)
                {
                    std::thread([target]() {
                        auto dialog = std::make_unique<horizon::PrintDialog>(target);
                        dialog->run();
                    }).detach();
                }
                else
                {
                    LOG_INFO << "WaylandWindow: No print target found";
                }
            });

        signal_manager.connect(
            "redo",
            [this](SignalContext &)
            {
                LOG_INFO << "WaylandWindow: Received 'redo' signal from menu/IPC";
                auto *target = find_undo_target();
                if (target)
                {
                    EventContext ctx;
                    ctx.sender = target;
                    target->when_redo.run(ctx);
                }
                else
                {
                    LOG_INFO << "WaylandWindow: No redo target found";
                }
            });

        signal_manager.connect(
            "zoom_in",
            [this](SignalContext &)
            {
                LOG_INFO << "WaylandWindow: Received 'zoom_in' signal from menu/IPC";
                auto *target = find_zoom_target();
                if (target)
                {
                    EventContext ctx;
                    ctx.sender = target;
                    target->when_zoom_in.run(ctx);
                }
                else
                {
                    LOG_INFO << "WaylandWindow: No zoom_in target found";
                }
            });

        signal_manager.connect(
            "zoom_out",
            [this](SignalContext &)
            {
                LOG_INFO << "WaylandWindow: Received 'zoom_out' signal from menu/IPC";
                auto *target = find_zoom_target();
                if (target)
                {
                    EventContext ctx;
                    ctx.sender = target;
                    target->when_zoom_out.run(ctx);
                }
                else
                {
                    LOG_INFO << "WaylandWindow: No zoom_out target found";
                }
            });

        signal_manager.connect("file.open",
                               [this](SignalContext &)
                               {
                                   LOG_INFO << "WaylandWindow: Received 'file.open' signal";
                                   if (Window *win = find_window_target(m_root.get()))
                                   {
                                       std::thread([this, win]() {
                                           auto dialog = std::make_unique<FileDialog>(
                                               FileDialogMode::Open, i18n().tr("core.global_menu.file_open"));

                                           dialog->when_accepted.connect(
                                               [win](FileDialogAcceptedContext &ctx)
                                               {
                                                   Window::FileOpenedContext fctx;
                                                   fctx.path = ctx.selected_path;
                                                   win->when_file_opened.run(fctx);
                                                   win->signals.emit("file.opened", &fctx);
                                               });

                                           dialog->run();
                                       }).detach();
                                   }
                               });

        signal_manager.connect("file.open_folder",
                               [this](SignalContext &)
                               {
                                   LOG_INFO << "WaylandWindow: Received 'file.open_folder' signal";
                                   if (Window *win = find_window_target(m_root.get()))
                                   {
                                       std::thread([this, win]() {
                                           auto dialog = std::make_unique<FileDialog>(
                                               FileDialogMode::SelectFolder,
                                               i18n().tr("core.global_menu.file_open_folder"));

                                           dialog->when_accepted.connect(
                                               [win](FileDialogAcceptedContext &ctx)
                                               {
                                                   Window::FileOpenedContext fctx;
                                                   fctx.path = ctx.selected_path;
                                                   win->when_folder_opened.run(fctx);
                                                   win->signals.emit("folder.opened", &fctx);
                                               });

                                           dialog->run();
                                       }).detach();
                                   }
                               });

        signal_manager.connect("file.save",
                               [this](SignalContext &)
                               {
                                   LOG_INFO << "WaylandWindow: Received 'file.save' signal";
                                   if (Window *win = find_window_target(m_root.get()))
                                   {
                                       std::string current_path = win->current_file_path();
                                       if (!current_path.empty())
                                       {
                                           Window::FileSaveContext sctx;
                                           sctx.path = current_path;
                                           win->when_save.run(sctx);
                                           win->signals.emit("file.saved", &sctx);
                                       }
                                       else
                                       {
                                           // Fallback to Save As
                                           SignalContext empty_ctx;
                                           this->signal_manager.emit("file.save_as", empty_ctx);
                                       }
                                   }
                               });

        signal_manager.connect("file.save_as",
                               [this](SignalContext &)
                               {
                                   LOG_INFO << "WaylandWindow: Received 'file.save_as' signal";
                                   if (Window *win = find_window_target(m_root.get()))
                                   {
                                       std::thread([this, win]() {
                                           auto dialog = std::make_unique<FileDialog>(
                                               FileDialogMode::SaveAs, i18n().tr("core.global_menu.file_save_as"));

                                           // Pre-fill path if available
                                           std::string current_path = win->current_file_path();
                                           if (!current_path.empty())
                                           {
                                               dialog->set_current_path(current_path);
                                           }

                                           dialog->when_accepted.connect(
                                               [win](FileDialogAcceptedContext &ctx)
                                               {
                                                   Window::FileSaveContext sctx;
                                                   sctx.path = ctx.selected_path;
                                                   win->when_save_as.run(sctx);
                                                   win->signals.emit("file.saved_as", &sctx);
                                               });

                                           dialog->run();
                                       }).detach();
                                   }
                               });

        signal_manager.connect("file.close",
                               [this](SignalContext &)
                               {
                                   LOG_INFO << "WaylandWindow: Received 'file.close' signal";
                                   if (Window *win = find_window_target(m_root.get()))
                                   {
                                       EventContext ctx;
                                       ctx.sender = win;
                                       win->when_file_close.run(ctx);
                                       win->signals.emit("file.close");
                                   }
                               });

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
                                            else if (signal == "menu_item_clicked" &&
                                                     !token.empty())
                                            {
                                                LOG_INFO << "[APP] Handling menu_item_clicked: "
                                                         << token;
                                                this->signal_manager.emit(token, nullptr);
                                            }
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

            // 1. Execute posted tasks FIRST so their invalidations are caught this frame
            {
                std::deque<std::function<void()>> tasks;
                {
                    std::lock_guard<std::mutex> lock(m_task_mutex);
                    std::swap(tasks, m_task_queue);
                }
                for (auto &task : tasks)
                {
                    if (task) task();
                }
            }

            wl_display_dispatch_pending(m_surface->display());

            // 2. Determine if we need to render
            uint64_t frame_now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now().time_since_epoch())
                                     .count();
            static constexpr uint64_t FRAME_MS = 16;
            
            bool root_dirty = (m_root && (m_root->is_dirty() || m_root->is_child_dirty()));
            bool has_pending = m_full_repaint || !m_dirty_widgets.empty() || root_dirty;

            if (has_pending && !m_is_minimized && (m_surface->is_configured() || m_first_frame))
            {
                if (m_surface->width() > 0 && m_surface->height() > 0 && 
                    (frame_now - m_last_commit_time) >= FRAME_MS)
                {
                    bool should_render = true; // We already checked has_pending
                    
                    if (should_render && m_root)
                    {
                        m_dirty_widgets.clear();
                        bool full = m_full_repaint || m_first_frame;
                        m_full_repaint = false;

                        if (m_surface)
                        {
                            eglMakeCurrent(m_surface->egl_display(), m_surface->egl_surface(),
                                           m_surface->egl_surface(), m_surface->egl_context());
                        }

                        if (is_transparent_surface())
                            glClearColor(0, 0, 0, 0);
                        else
                            glClearColor(0, 0, 0, 1);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        m_gl_queue.clear();

                        if (m_surface->data())
                        {
                            CairoGraphicContext ctx(this, m_surface->data(), m_surface->width(),
                                                    m_surface->height());


                            if (is_transparent_surface())
                            {
                                ctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                                ctx.clearRect(0, 0, m_surface->width(), m_surface->height());
                            }

                            m_root->render(ctx, 0, 0, m_surface->width(), m_surface->height(),
                                           full || true); // Force full for now to stabilize
                            ctx.flush();
                        }

                        if (m_popup_menu && m_popup_surface && m_popup_surface->data())
                        {
                            CairoGraphicContext pctx(this, m_popup_surface->data(),
                                                     m_popup_surface->width(),
                                                     m_popup_surface->height());
                            pctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                            pctx.clearRect(0, 0, m_popup_surface->width(),
                                           m_popup_surface->height());

                            m_popup_menu->render(pctx, 0, 0, m_popup_surface->width(),
                                                 m_popup_surface->height(), true);
                            pctx.flush();
                            render_gl_popup();
                        }

                        if (m_popup_vault && m_popup_surface && m_popup_surface->data())
                        {
                            CairoGraphicContext pctx(this, m_popup_surface->data(),
                                                     m_popup_surface->width(),
                                                     m_popup_surface->height());
                            pctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                            pctx.clearRect(0, 0, m_popup_surface->width(),
                                           m_popup_surface->height());

                            m_popup_vault->render(pctx, 0, 0, m_popup_surface->width(),
                                                 m_popup_surface->height(), true);
                            pctx.flush();
                            render_gl_vault();
                        }

                        if (m_tooltip_widget && m_tooltip_surface && m_tooltip_surface->data())
                        {
                            CairoGraphicContext tctx(this, m_tooltip_surface->data(),
                                                     m_tooltip_surface->width(),
                                                     m_tooltip_surface->height());
                            tctx.setColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
                            tctx.clearRect(0, 0, m_tooltip_surface->width(),
                                           m_tooltip_surface->height());

                            m_tooltip_widget->render(tctx, 0, 0, m_tooltip_surface->width(),
                                                     m_tooltip_surface->height(), true);
                            tctx.flush();
                            render_gl_tooltip();
                        }

                        render_gl_ui();
                        m_last_commit_time = frame_now;
                        m_first_frame = false;
                    }
                }
            }

                // IMPORTANT: Wayland thread-safety requires prepare_read BEFORE poll.
                // We do this AFTER render_gl_ui so we don't hold the read lock during
                // eglSwapBuffers.
                while (wl_display_prepare_read(m_surface->display()) != 0)
                {
                    wl_display_dispatch_pending(m_surface->display());
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
                    bool timers_empty = false;
                    {
                        std::lock_guard<std::mutex> lock(m_state_mutex);
                        timers_empty = m_timers.empty();
                    }

                    // Check nearest timer expiry
                    if (!timers_empty)
                    {
                        uint64_t next_expiry = 0;
                        {
                            std::lock_guard<std::mutex> lock(m_state_mutex);
                            for (const auto &[id, timer] : m_timers)
                            {
                                if (next_expiry == 0 || timer.next_expiry < next_expiry)
                                    next_expiry = timer.next_expiry;
                            }
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

                    // Redraw heartbeat: if we have pending redraws but were rate-limited,
                    // wake up exactly when 16ms have passed.
                    bool has_pending = m_full_repaint || !m_dirty_widgets.empty();
                    if (has_pending)
                    {
                        static constexpr uint64_t FRAME_MS = 16;
                        uint64_t elapsed = now - m_last_commit_time;
                        int redraw_ms = (elapsed >= FRAME_MS) ? 0 : (int)(FRAME_MS - elapsed);
                        timeout = (timeout == -1) ? redraw_ms : std::min(timeout, redraw_ms);
                    }
                }

                if (timeout == -1 || timeout > 100)
                    timeout = 100; // Never block for more than 100ms

                int ret = poll(fds, 2, timeout);
                if (ret < 0)
                {
                    wl_display_cancel_read(m_surface->display());
                    if (errno == EINTR)
                        continue;
                    LOG_ERROR << "[APP] poll() error: " << strerror(errno);
                    m_is_running = false;
                    break;
                }

                if (fds[0].revents & POLLIN)
                {
                    wl_display_read_events(m_surface->display());
                }
                else
                {
                    wl_display_cancel_read(m_surface->display());
                }

                wl_display_dispatch_pending(m_surface->display());

                now = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();

                std::vector<std::function<void()>> callbacks_to_run;
                {
                    std::lock_guard<std::mutex> lock(m_state_mutex);
                    for (auto it = m_timers.begin(); it != m_timers.end();)
                    {
                        if (now >= it->second.next_expiry)
                        {
                            if (it->second.callback)
                                callbacks_to_run.push_back(it->second.callback);

                            if (it->second.repeat)
                            {
                                it->second.next_expiry = now + it->second.interval_ms;
                                ++it;
                            }
                            else
                            {
                                it = m_timers.erase(it);
                            }
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }

                for (auto &callback : callbacks_to_run)
                {
                    callback();
                }

                // Trigger cursor blink independently of timers
                if (m_focused && !m_is_repeating && (now - m_blink_last_time >= 500))
                {
                    m_blink_last_time = now;
                    m_focused->invalidate();
                }

                if (ret > 0)
                {
                    if (fds[1].revents & POLLIN)
                    {
                        uint64_t val;
                        if (read(m_wakeup_fd, &val, sizeof(val)) < 0)
                        {
                            // ignore error
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

    void WaylandWindow::set_global_menu(const std::vector<Menu *> &menus)
    {
        m_global_menus = menus;
    }

    namespace
    {
        Widget *detect_clipboard_target(Widget *root)
        {
            if (!root)
                return nullptr;
            if (root->supports_clipboard())
                return root;
            for (const auto &child : root->children())
            {
                auto *found = detect_clipboard_target(child.get());
                if (found)
                    return found;
            }
            return nullptr;
        }

        class WidgetDataSink : public DataSink
        {
            Widget *m_widget;
            std::string m_mime;
            std::vector<uint8_t> m_buffer;

        public:
            WidgetDataSink(Widget *w, const std::string &mime) : m_widget(w), m_mime(mime) {}
            void write(const std::vector<uint8_t> &data) override
            {
                m_buffer.insert(m_buffer.end(), data.begin(), data.end());
            }
            void done() override
            {
                if (m_widget)
                    m_widget->on_clipboard_data_received(m_mime, std::move(m_buffer));
            }
            void error() override {}
        };
    } // namespace

    Menu *WaylandWindow::get_menu(const std::string &id) const
    {
        for (const auto &menu : m_menues)
        {
            if (menu->id() == id)
                return menu.get();
        }
        return nullptr;
    }

    void WaylandWindow::update_global_menu()
    {
        if (m_client_menu && m_is_activated)
        {
            m_client_menu->set_global_menu(m_global_menus);
        }
    }

    void WaylandWindow::init_global_menu()
    {
        if (!m_use_global_menu)
            return;

        m_app_menu->set_title(m_name);
        m_app_menu->set_bold(true);

        if (m_preferences_factory)
        {
            auto *pref_item =
                m_app_menu->add_item(i18n().tr("core.global_menu.preferences"), "Ctrl+,");
            pref_item->set_id("preferences");
        }

        auto *ab_item = m_app_menu->add_item(i18n().tr("core.global_menu.aboutus"), "Ctrl+H");
        ab_item->set_id("aboutus");

        m_app_menu->add_separator();
        auto *global_quit = m_app_menu->add_item(i18n().tr("core.global_menu.quit"), "Ctrl+Q");
        global_quit->set_id("quit");
        m_app_menu->set_id("app");

        // Automatic File Menu detection
        Window *file_win = find_window_target(m_root.get());
        bool has_print = detect_print_support(m_root.get()); LOG_INFO << "WaylandWindow: has_print = " << has_print;

        if (file_win || has_print)
        {
            uint32_t caps = file_win ? file_win->file_capabilities() : 0;
            Menu *file_menu = get_menu("file");
            bool is_new = false;

            if (!file_menu)
            {
                auto new_menu = std::make_unique<Menu>();
                new_menu->set_title(i18n().tr("core.global_menu.file"));
                new_menu->set_id("file");
                file_menu = new_menu.get();
                m_menues.push_back(std::move(new_menu));
                is_new = true;
            }

            bool has_open = (caps & FileOpen) || (caps & FileOpenFolder);
            bool has_save_close = (caps & FileClose) || (caps & FileSave) || (caps & FileSaveAs);

            if (caps & FileOpen)
            {
                file_menu->add_item(i18n().tr("core.global_menu.file_open"), "Ctrl+O", "file.open");
            }
            if (caps & FileOpenFolder)
            {
                file_menu->add_item(i18n().tr("core.global_menu.file_open_folder"), "Ctrl+Shift+O",
                                    "file.open_folder");
            }

            if (has_print)
            {
                if (has_open) file_menu->add_separator();
                file_menu->add_item(i18n().tr("core.global_menu.print"), "Ctrl+P", "print");
            }

            if (has_save_close && (has_open || has_print))
            {
                file_menu->add_separator();
            }

            if (caps & FileClose)
            {
                file_menu->add_item(i18n().tr("core.global_menu.file_close"), "Ctrl+W",
                                    "file.close");
            }

            if (caps & FileSave)
            {
                file_menu->add_item(i18n().tr("core.global_menu.file_save"), "Ctrl+S", "file.save");
            }

            if (caps & FileSaveAs)
            {
                file_menu->add_item(i18n().tr("core.global_menu.file_save_as"), "Ctrl+Shift+S",
                                    "file.save_as");
            }

            if (is_new)
            {
                // We want File menu to be right after App menu.
                // Since App menu will be inserted at 0 later, we insert at 0 now.
                m_global_menus.insert(m_global_menus.begin(), file_menu);
            }
        }

        // Automatic Edit Menu detection & merging
        bool has_clipboard = detect_clipboard_target(m_root.get());
        bool has_undo = detect_undo_support(m_root.get());

        if (has_clipboard || has_undo)
        {
            Menu *edit_menu = get_menu("edit");
            bool is_new = false;

            if (!edit_menu)
            {
                auto new_menu = std::make_unique<Menu>();
                new_menu->set_title(i18n().tr("core.global_menu.edit"));
                new_menu->set_id("edit");
                edit_menu = new_menu.get();
                m_menues.push_back(std::move(new_menu));
                is_new = true;
            }
            else
            {
                // If it already has items, add a separator before we insert more
                if (!edit_menu->children().empty())
                {
                    edit_menu->add_separator();
                }
            }

            if (has_undo)
            {
                edit_menu->add_item(i18n().tr("core.global_menu.undo"), "Ctrl+Z", "undo");
                edit_menu->add_item(i18n().tr("core.global_menu.redo"), "Ctrl+Shift+Z", "redo");
                
                if (has_clipboard)
                {
                    edit_menu->add_separator();
                }
            }

            if (has_clipboard)
            {
                edit_menu->add_item(i18n().tr("core.global_menu.copy"), "Ctrl+C", "copy");
                edit_menu->add_item(i18n().tr("core.global_menu.cut"), "Ctrl+X", "cut");
                edit_menu->add_item(i18n().tr("core.global_menu.paste"), "Ctrl+V", "paste");
            }

            if (is_new)
            {
                m_global_menus.push_back(edit_menu);
            }
        }

        // Automatic Visualization Menu detection & merging
        bool has_fullscreen = detect_fullscreen_support(m_root.get());
        bool has_zoom = detect_zoom_support(m_root.get());

        if (has_fullscreen || has_zoom)
        {
            Menu *vis_menu = get_menu("view");
            bool is_new = false;

            if (!vis_menu)
            {
                auto new_menu = std::make_unique<Menu>();
                new_menu->set_title(i18n().tr("core.global_menu.view"));
                new_menu->set_id("view");
                vis_menu = new_menu.get();
                m_menues.push_back(std::move(new_menu));
                is_new = true;
            }
            else
            {
                if (!vis_menu->children().empty())
                {
                    vis_menu->add_separator();
                }
            }

            if (has_zoom)
            {
                vis_menu->add_item(i18n().tr("core.global_menu.zoom_in"), "Ctrl++", "zoom_in");
                vis_menu->add_item(i18n().tr("core.global_menu.zoom_out"), "Ctrl+-", "zoom_out");
                
                if (has_fullscreen)
                {
                    vis_menu->add_separator();
                }
            }

            if (has_fullscreen)
            {
                auto *item = vis_menu->add_item(i18n().tr("core.global_menu.fullscreen"), "F11");
                item->set_id("fullscreen");
            }

            if (is_new)
            {
                m_global_menus.push_back(vis_menu);
            }
        }

        auto mnu = m_app_menu.get();
        if (std::find(m_global_menus.begin(), m_global_menus.end(), mnu) == m_global_menus.end())
        {
            m_global_menus.insert(m_global_menus.begin(), mnu);
        }

        if (!m_client_menu)
        {
            m_client_menu = std::make_shared<ClientMenu>();
        }
        if (m_client_menu && m_is_activated)
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
        auto it =
            std::find_if(m_menues.begin(), m_menues.end(),
                         [&title](const std::unique_ptr<Menu> &m) { return m->title() == title; });

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
            m_global_menus.erase(std::remove(m_global_menus.begin(), m_global_menus.end(), old_ptr),
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

        if (m_popup_listener)
        {
            // If the main window receives a pointer event while a non-grabbing popup (like Vault)
            // is open, it means the user clicked OUTSIDE the popup.
            // (If they clicked inside the popup, the compositor would deliver the event to the popup surface).
            if (event.type == PointerEvent::Type::Press)
            {
                LOG_INFO << "[WINDOW] Click outside popup detected on main window. Closing popup.";
                if (m_popup_menu) close_context_menu();
                else close_vault();
            }
            // Consume the event so we don't accidentally click a button underneath the popup
            return;
        }

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
            }
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

        if (width <= 0 || height <= 0)
            return;

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

        if (m_client_menu && m_is_running && m_use_global_menu)
        {
            if (active)
                m_client_menu->set_global_menu(m_global_menus);
            else
                m_client_menu->set_global_menu({});
        }

        if (active && m_is_minimized)
        {
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

    void WaylandWindow::on_drag_drop_event(const DragDropEvent &event)
    {
        if (!m_root)
            return;

        static Widget *drag_hovered = nullptr;

        switch (event.type)
        {
        case DragDropEvent::Type::Enter:
        case DragDropEvent::Type::Motion:
        {
            Widget *under = m_root->hit_test((int)event.x, (int)event.y);

            if (under != drag_hovered)
            {
                if (drag_hovered)
                {
                    DragEventContext leave_ctx;
                    leave_ctx.sender = drag_hovered;
                    drag_hovered->when_drag_leave.run(leave_ctx);
                }

                // Look for a widget that accepts drops in the hierarchy
                Widget *target = under;
                while (target && !target->accept_drops())
                {
                    target = target->parent();
                }
                drag_hovered = target;

                if (drag_hovered)
                {
                    DragEventContext enter_ctx;
                    enter_ctx.sender = drag_hovered;
                    enter_ctx.x = event.x;
                    enter_ctx.y = event.y;
                    enter_ctx.mime_types = event.mime_types;
                    drag_hovered->when_drag_enter.run(enter_ctx);
                }
            }

            if (drag_hovered)
            {
                DragEventContext over_ctx;
                over_ctx.sender = drag_hovered;
                over_ctx.x = event.x;
                over_ctx.y = event.y;
                over_ctx.mime_types = event.mime_types;
                drag_hovered->when_drag_over.run(over_ctx);

                // Feedback to compositor about acceptance
                struct wl_data_offer *offer = static_cast<struct wl_data_offer *>(event.data_offer);
                if (offer)
                {
                    // Accept the first supported mime type for now
                    const char *mime = event.mime_types.empty() ? nullptr : event.mime_types[0].c_str();
                    wl_data_offer_accept(offer, event.serial, mime);
                    wl_data_offer_set_actions(offer, 
                        WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY | WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE,
                        WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
                }
            }
            else
            {
                // Inform compositor that we don't accept it here
                struct wl_data_offer *offer = static_cast<struct wl_data_offer *>(event.data_offer);
                if (offer)
                {
                    wl_data_offer_accept(offer, event.serial, nullptr);
                }
            }
            break;
        }

        case DragDropEvent::Type::Leave:
            if (drag_hovered)
            {
                DragEventContext leave_ctx;
                leave_ctx.sender = drag_hovered;
                drag_hovered->when_drag_leave.run(leave_ctx);
                drag_hovered = nullptr;
            }
            break;

        case DragDropEvent::Type::Drop:
            if (drag_hovered && drag_hovered->accept_drops())
            {
                struct wl_data_offer *offer = static_cast<struct wl_data_offer *>(event.data_offer);
                if (!offer) {
                    break;
                }

                DropEventContext drop_ctx;
                drop_ctx.sender = drag_hovered;
                drop_ctx.mime_types = event.mime_types;

                drop_ctx.m_data_fetcher = [this, offer](const std::string &mime) -> std::vector<uint8_t>
                {
                    // Internal bypass: if source is in the same process, fetch directly from memory
                    if (s_active_drag_source && s_active_drag_source->m_current_fetcher)
                    {
                        return s_active_drag_source->m_current_fetcher(mime);
                    }

                    // External drag: use standard Wayland pipe
                    int fds[2];
                    if (pipe(fds) < 0) return {};

                    wl_data_offer_receive(offer, mime.c_str(), fds[1]);
                    wl_display_flush(m_surface->display());
                    close(fds[1]);

                    std::vector<uint8_t> result;
                    uint8_t buffer[4096];
                    ssize_t n;
                    // For external drags, this read is safe because the sender is another process
                    while ((n = read(fds[0], buffer, sizeof(buffer))) > 0)
                    {
                        result.insert(result.end(), buffer, buffer + n);
                    }
                    close(fds[0]);
                    return result;
                };

                drag_hovered->when_drop.run(drop_ctx);
                
                // Signal completion to compositor
                if (wl_proxy_get_version((struct wl_proxy *)offer) >= 3) {
                    wl_data_offer_finish(offer);
                }
            }
            drag_hovered = nullptr;
            break;
        }
    }

    void WaylandWindow::on_clipboard_selection(void *offer)
    {
        if (m_clipboard_backend)
        {
            m_clipboard_backend->handle_selection((struct wl_data_offer *)offer);
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
        /* if (event.key == KEY_ESC)
        {
            quit();
            return;
        } */

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

        m_last_serial = event.serial;

        KeyEventContext new_ev;
        new_ev.sender = nullptr;
        new_ev.key = event.key;
        new_ev.modifiers = event.modifiers;
        new_ev.keysym = event.keysym;
        new_ev.serial = event.serial;
        new_ev.text = event.text;

        // Dispatch key event with bubbling
        Widget *current = target;
        while (current)
        {
            current->when_key_press.run(new_ev);
            if (new_ev.stop_propagation)
                return;
            current = current->parent();
        }


        // Focus navigation
        if (event.key == KEY_TAB)
        {
            if (event.modifiers & SHIFT)
            {
                focus_previous();
            }
            else
            {
                focus_next();
            }
            return;
        }

        // Standard shortcuts for clipboard: automatically dispatch if widget hierarchy supports it
        if ((m_modifiers & CTRL))
        {
            if (event.keysym == XKB_KEY_c || event.keysym == XKB_KEY_C)
            {
                SignalContext ctx;
                signal_manager.emit("copy", ctx);
                return;
            }
            else if (event.keysym == XKB_KEY_x || event.keysym == XKB_KEY_X)
            {
                SignalContext ctx;
                signal_manager.emit("cut", ctx);
                return;
            }
            else if (event.keysym == XKB_KEY_v || event.keysym == XKB_KEY_V)
            {
                SignalContext ctx;
                signal_manager.emit("paste", ctx);
                return;
            }
            else if (event.keysym == XKB_KEY_z || event.keysym == XKB_KEY_Z)
            {
                if (m_modifiers & SHIFT)
                {
                    SignalContext ctx;
                    signal_manager.emit("redo", ctx);
                }
                else
                {
                    SignalContext ctx;
                    signal_manager.emit("undo", ctx);
                }
                return;
            }
            else if (event.keysym == XKB_KEY_y || event.keysym == XKB_KEY_Y)
            {
                SignalContext ctx;
                signal_manager.emit("redo", ctx);
                return;
            }
            else if (event.keysym == XKB_KEY_plus || event.keysym == XKB_KEY_equal || event.keysym == XKB_KEY_KP_Add)
            {
                SignalContext ctx;
                signal_manager.emit("zoom_in", ctx);
                return;
            }
            else if (event.keysym == XKB_KEY_minus || event.keysym == XKB_KEY_KP_Subtract)
            {
                SignalContext ctx;
                signal_manager.emit("zoom_out", ctx);
                return;
            }
        }

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

        if ((m_modifiers & CTRL) && (event.keysym == XKB_KEY_p || event.keysym == XKB_KEY_P))
        {
            SignalContext ctx;
            signal_manager.emit("print", ctx);
            return;
        }

        Widget *target = m_focused ? m_focused : m_root.get();

        KeyEventContext new_ev;
        new_ev.sender = nullptr;
        new_ev.key = event.key;
        new_ev.modifiers = event.modifiers;
        new_ev.keysym = event.keysym;
        new_ev.text = event.text;
        Widget *current = target;
        while (current)
        {
            current->when_key_release.run(new_ev);
            if (new_ev.stop_propagation)
                return;
            current = current->parent();
        }
    }

    void WaylandWindow::PopupEventListener::on_pointer_event(const PointerEvent &event)
    {
        if (!m_window || (!m_window->m_popup_menu && !m_window->m_popup_vault))
        {
            return;
        }

        Widget *popup_root = m_window->m_popup_menu ? (Widget*)m_window->m_popup_menu : (Widget*)m_window->m_popup_vault;

        // LOG_INFO << "[POPUP_EV] Type: " << (int)event.type << " at (" << event.x << ", " << event.y << ") serial: " << event.serial;

        int x = (int)event.x;
        int y = (int)event.y;

        Widget *under = popup_root->hit_test(x, y);

        // 0. Click-outside logic: If we are clicking on the main window (not the popup)
        // while a popup is active, we should close the popup and let the main window
        // handle the click normally.
        if (!under && event.type == PointerEvent::Type::Press)
        {
            LOG_INFO << "[POPUP] Click outside. Click=(" << x << "," << y 
                     << ") Vault bounds: x=" << popup_root->x() << " y=" << popup_root->y() 
                     << " w=" << popup_root->width() << " h=" << popup_root->height();
            if (m_window->m_popup_menu) m_window->close_context_menu();
            else m_window->close_vault();
            return;
        }

        // 1. Hover tracking (Enter/Leave/Move)
        if (event.type == PointerEvent::Type::Move || event.type == PointerEvent::Type::Enter || event.type == PointerEvent::Type::Leave)
        {
            if (event.type == PointerEvent::Type::Leave) under = nullptr;

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

        // 2. Scroll Dispatching (Independent of 'under' hit-test for robustness)
        if (event.type == PointerEvent::Type::Scroll)
        {
            MouseWheelEventContext ev;
            ev.dx = event.dx;
            ev.dy = event.dy;
            ev.x = (double)x;
            ev.y = (double)y;
            ev.modifiers = m_window->m_modifiers;
            
            std::vector<Widget *> chain;
            Widget *temp = under ? under : popup_root;
            while (temp)
            {
                chain.push_back(temp);
                temp = temp->parent();
            }

            for (Widget *w : chain)
            {
                ev.sender = w;
                w->when_mouse_wheel.run(ev);
                if (ev.stop_propagation) {
                    break;
                }
            }
            m_window->invalidate();
        }
        else
        {
            // If we have a pressed widget, route events to it even if mouse moved outside it
            Widget *target = m_pressed ? m_pressed : under;
            if (!target) return;

            std::vector<Widget *> chain;
            Widget *temp = target;
            while (temp)
            {
                chain.push_back(temp);
                temp = temp->parent();
            }

            if (event.type == PointerEvent::Type::Move || event.type == PointerEvent::Type::Enter)
            {
                MouseMoveEventContext mv;
                mv.x = (double)x;
                mv.y = (double)y;
                mv.modifiers = m_window->m_modifiers;

                if (!m_pressed_buttons.empty() && m_pressed) {
                    for (Widget *w : chain) {
                        mv.sender = w;
                        w->when_mouse_drag.run(mv);
                        if (mv.stop_propagation) break;
                    }
                } else {
                    for (Widget *w : chain) {
                        mv.sender = w;
                        w->when_mouse_move.run(mv);
                        if (mv.stop_propagation) break;
                    }
                }
            }
            else if (event.type == PointerEvent::Type::Press)
            {
                m_pressed_buttons.insert(event.button);
                m_pressed = under; // Capture the pressed widget for drag/release
                
                // Allow interactive widgets (like TextBox) inside Vaults to receive keyboard focus
                m_window->set_focused_widget(under);

                MouseButtonEventContext ev;
                ev.button = event.button;
                ev.x = (double)x;
                ev.y = (double)y;
                ev.modifiers = m_window->m_modifiers;
                ev.serial = event.serial;
                for (Widget *w : chain)
                {
                    ev.sender = w;
                    // Adjust Y for scrolled menus so bounds checks pass in Widget.cpp
                    if (auto *m = dynamic_cast<Menu *>(w->parent()))
                    {
                        ev.y = (double)y + m->scroll_y();
                    }
                    else
                    {
                        ev.y = (double)y;
                    }
                    w->when_mouse_press.run(ev);
                    if (ev.stop_propagation)
                        break;
                }
            }
            else if (event.type == PointerEvent::Type::Release)
            {
                auto it = m_pressed_buttons.find(event.button);
                if (it == m_pressed_buttons.end())
                {
                    LOG_INFO << "[POPUP] Ignoring leaked release for button: " << event.button;
                    return;
                }
                m_pressed_buttons.erase(it);

                MouseButtonEventContext ev;
                ev.button = event.button;
                ev.x = (double)x;
                ev.y = (double)y;
                ev.modifiers = m_window->m_modifiers;
                ev.serial = event.serial;
                ev.stop_propagation = true; // IMPORTANT: Prevent propagation to main window

                // IMPORTANT: Close the menu BEFORE running the handlers.
                WaylandWindow *win = m_window;
                uint32_t mods = win->m_modifiers;

                if (win->m_popup_menu) {
                    win->close_context_menu();
                }
                
                for (Widget *w : chain)
                {
                    ev.sender = w;
                    ev.modifiers = mods;
                    if (auto *m = dynamic_cast<Menu *>(w->parent()))
                    {
                        ev.y = (double)y + m->scroll_y();
                    }
                    else
                    {
                        ev.y = (double)y;
                    }
                    w->when_mouse_release.run(ev);
                }

                if (m_pressed_buttons.empty()) {
                    m_pressed = nullptr; // Clear pressed widget state
                }
            }
        }

    }

    void WaylandWindow::PopupEventListener::on_close()
    {
        m_window->close_context_menu();
        m_window = nullptr;
    }

    void WaylandWindow::set_override_cursor(CursorType type)
    {
        m_override_cursor = type;
        if (m_surface)
        {
            m_surface->set_cursor(type);
            // Flush immediately so the compositor shows the cursor before any blocking call
            wl_display_flush(m_surface->display());
        }
    }

    void WaylandWindow::clear_override_cursor()
    {
        m_override_cursor.reset();
        // Restore cursor based on current hover state
        if (m_surface)
        {
            if (m_hovered)
                m_surface->set_cursor(m_hovered->cursor_type());
            else
                m_surface->set_cursor(CursorType::Default);
            wl_display_flush(m_surface->display());
        }
    }

    void WaylandWindow::handle_move(const PointerEvent &event)
    {
        if (!m_root)
            return;

        m_pointer_x = event.x;
        m_pointer_y = event.y;

        // If an override cursor is active, apply it and skip all hover logic
        if (m_override_cursor.has_value())
        {
            if (m_surface)
                m_surface->set_cursor(m_override_cursor.value());
            return;
        }

        // Detect resize edge
        uint32_t edge = XDG_TOPLEVEL_RESIZE_EDGE_NONE;
        if (!is_maximized() && m_resizable)
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

                for (Widget *w : old_path)
                {
                    if (std::find(new_path.begin(), new_path.end(), w) == new_path.end())
                    {
                        EventContext leave_ev;
                        leave_ev.sender = w;
                        w->when_mouse_leave.run(leave_ev);
                    }
                }

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

            if (m_pressed)
            {
                if (!m_is_dragging)
                {
                    double dx = event.x - m_drag_start_x;
                    double dy = event.y - m_drag_start_y;
                    double dist = std::sqrt(dx * dx + dy * dy);

                    if (dist > 8.0)
                    {
                        // Look for a draggable widget in the hierarchy
                        Widget *draggable_widget = m_pressed;
                        while (draggable_widget && !draggable_widget->is_draggable())
                        {
                            draggable_widget = draggable_widget->parent();
                        }

                        if (draggable_widget)
                        {
                            m_is_dragging = true;
                            
                            DragEventContext drag_ev;
                            drag_ev.sender = draggable_widget;
                            drag_ev.x = event.x;
                            drag_ev.y = event.y;
                            draggable_widget->when_drag_start.run(drag_ev);
                            
                            // Once DND starts, we stop processing normal mouse events for this press
                            m_pressed = nullptr;
                            return;
                        }
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
        }

        // Push final cursor state to surface
        // Override cursor always takes absolute priority
        if (m_override_cursor.has_value())
        {
            m_surface->set_cursor(m_override_cursor.value());
        }
        else if (m_resize_edge != 0)
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

        if (m_popup_menu)
        {
            close_context_menu();
        }

        if (m_resize_edge != XDG_TOPLEVEL_RESIZE_EDGE_NONE)
        {
            m_surface->request_resize(event.serial, m_resize_edge);
            return;
        }

        Widget *under = m_root->hit_test(event.x, event.y);

        if (under)
        {
            m_pressed = under;
            m_drag_start_x = event.x;
            m_drag_start_y = event.y;
            m_is_dragging = false;

            // Update focus
            set_focused_widget(under);

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
                if (!is_widget_alive(w))
                    continue;

                new_ev.sender = w;
                w->when_mouse_press.run(new_ev);
                if (new_ev.stop_propagation)
                    break;
            }
        }
        else
        {
            set_focused_widget(nullptr);
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
            if (!is_widget_alive(w))
                continue;

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
            if (is_fullscreen())
                return;

            // 1. Identify target
            Widget *target = m_focused;
            if (!target || !target->supports_fullscreen())
            {
                target = find_fullscreen_target(m_root.get());
            }

            if (!target)
                return;

            // 2. Apply isolation
            apply_fullscreen_isolation(target);

            // 3. Hide titlebar if we are a Window
            if (Window *win = dynamic_cast<Window *>(m_root.get()))
            {
                if (win->titlebar())
                {
                    win->titlebar()->set_visible(false);
                    m_hidden_by_fullscreen.push_back(win->titlebar());
                }
            }

            m_compositor_context->fullscreen();

            // 4. Trigger event
            FullscreenEventContext ev;
            ev.sender = target;
            ev.width = m_surface->width(); // This might be updated after resize
            ev.height = m_surface->height();
            target->when_enter_fullscreen.run(ev);

            invalidate();
        }
    }

    void WaylandWindow::unfullscreen()
    {
        if (m_compositor_context)
        {
            if (!is_fullscreen())
                return;

            m_compositor_context->unfullscreen();

            // Restore isolation
            restore_fullscreen_isolation();

            if (m_fullscreen_target)
            {
                FullscreenEventContext ev;
                ev.sender = m_fullscreen_target;
                ev.width = m_surface->width();
                ev.height = m_surface->height();
                m_fullscreen_target->when_leave_fullscreen.run(ev);
            }

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
            LOG_ERROR << "Shader compilation failed:\n" << info << "\nSource:\n" << source;
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
        GLint grad_start_loc = glGetUniformLocation(m_gl_program, "u_gradient_start");
        glUniform1f(grad_start_loc, 1.0f);
        GLint grad_end_loc = glGetUniformLocation(m_gl_program, "u_gradient_end");
        glUniform1f(grad_end_loc, 1.0f);
        GLint grad_horiz_loc = glGetUniformLocation(m_gl_program, "u_gradient_horizontal");
        glUniform1f(grad_horiz_loc, 0.0f);

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
            glUniform1f(grad_start_loc, draw.gradient_start);
            glUniform1f(grad_end_loc, draw.gradient_end);
            glUniform1f(grad_horiz_loc, draw.gradient_horizontal ? 1.0f : 0.0f);
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

    void WaylandWindow::render_gl_popup()
    {
        if (!m_popup_surface || !m_popup_surface->data())
            return;

        eglMakeCurrent(m_popup_surface->egl_display(), m_popup_surface->egl_surface(),
                       m_popup_surface->egl_surface(), m_popup_surface->egl_context());

        init_gl_resources();

        glViewport(0, 0, m_popup_surface->width(), m_popup_surface->height());
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m_gl_program);

        float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        GLint mvp_loc = glGetUniformLocation(m_gl_program, "u_mvp");
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, identity);

        GLint opacity_loc = glGetUniformLocation(m_gl_program, "u_opacity");
        glUniform1f(opacity_loc, 1.0f);
        GLint grad_start_loc = glGetUniformLocation(m_gl_program, "u_gradient_start");
        glUniform1f(grad_start_loc, 1.0f);
        GLint grad_end_loc = glGetUniformLocation(m_gl_program, "u_gradient_end");
        glUniform1f(grad_end_loc, 1.0f);
        GLint grad_horiz_loc = glGetUniformLocation(m_gl_program, "u_gradient_horizontal");
        glUniform1f(grad_horiz_loc, 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_gl_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_popup_surface->width(), m_popup_surface->height(),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, m_popup_surface->data());

        GLint pos_attr = glGetAttribLocation(m_gl_program, "position");
        GLint tex_attr = glGetAttribLocation(m_gl_program, "texcoord");

        glBindBuffer(GL_ARRAY_BUFFER, m_gl_vbo);
        glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        glEnableVertexAttribArray(pos_attr);
        glVertexAttribPointer(tex_attr, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(tex_attr);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        m_popup_surface->swap_buffers();

        // Restore main surface context
        eglMakeCurrent(m_surface->egl_display(), m_surface->egl_surface(), m_surface->egl_surface(),
                       m_surface->egl_context());
    }

    void WaylandWindow::render_gl_vault()
    {
        if (!m_popup_surface || !m_popup_surface->data() || !m_popup_vault)
            return;

        eglMakeCurrent(m_popup_surface->egl_display(), m_popup_surface->egl_surface(),
                       m_popup_surface->egl_surface(), m_popup_surface->egl_context());

        init_gl_resources();

        glViewport(0, 0, m_popup_surface->width(), m_popup_surface->height());
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m_gl_program);

        float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        GLint mvp_loc = glGetUniformLocation(m_gl_program, "u_mvp");
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, identity);

        GLint opacity_loc = glGetUniformLocation(m_gl_program, "u_opacity");
        glUniform1f(opacity_loc, 1.0f);
        GLint grad_start_loc = glGetUniformLocation(m_gl_program, "u_gradient_start");
        glUniform1f(grad_start_loc, 1.0f);
        GLint grad_end_loc = glGetUniformLocation(m_gl_program, "u_gradient_end");
        glUniform1f(grad_end_loc, 1.0f);
        GLint grad_horiz_loc = glGetUniformLocation(m_gl_program, "u_gradient_horizontal");
        glUniform1f(grad_horiz_loc, 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_gl_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_popup_surface->width(), m_popup_surface->height(),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, m_popup_surface->data());

        GLint pos_attr = glGetAttribLocation(m_gl_program, "position");
        GLint tex_attr = glGetAttribLocation(m_gl_program, "texcoord");

        glBindBuffer(GL_ARRAY_BUFFER, m_gl_vbo);
        glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        glEnableVertexAttribArray(pos_attr);
        glVertexAttribPointer(tex_attr, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(tex_attr);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        m_popup_surface->swap_buffers();

        // Restore main surface context
        eglMakeCurrent(m_surface->egl_display(), m_surface->egl_surface(), m_surface->egl_surface(),
                       m_surface->egl_context());
    }

    void WaylandWindow::render_gl_tooltip()
    {
        if (!m_tooltip_surface || !m_tooltip_surface->data())
            return;

        eglMakeCurrent(m_tooltip_surface->egl_display(), m_tooltip_surface->egl_surface(),
                       m_tooltip_surface->egl_surface(), m_tooltip_surface->egl_context());

        init_gl_resources();

        glViewport(0, 0, m_tooltip_surface->width(), m_tooltip_surface->height());
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m_gl_program);

        float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        GLint mvp_loc = glGetUniformLocation(m_gl_program, "u_mvp");
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, identity);

        GLint opacity_loc = glGetUniformLocation(m_gl_program, "u_opacity");
        glUniform1f(opacity_loc, 1.0f);
        GLint grad_start_loc = glGetUniformLocation(m_gl_program, "u_gradient_start");
        glUniform1f(grad_start_loc, 1.0f);
        GLint grad_end_loc = glGetUniformLocation(m_gl_program, "u_gradient_end");
        glUniform1f(grad_end_loc, 1.0f);
        GLint grad_horiz_loc = glGetUniformLocation(m_gl_program, "u_gradient_horizontal");
        glUniform1f(grad_horiz_loc, 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_gl_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_tooltip_surface->width(),
                     m_tooltip_surface->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     m_tooltip_surface->data());

        GLint pos_attr = glGetAttribLocation(m_gl_program, "position");
        GLint tex_attr = glGetAttribLocation(m_gl_program, "texcoord");

        glBindBuffer(GL_ARRAY_BUFFER, m_gl_vbo);
        glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        glEnableVertexAttribArray(pos_attr);
        glVertexAttribPointer(tex_attr, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(tex_attr);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        m_tooltip_surface->swap_buffers();

        // Restore main surface context
        eglMakeCurrent(m_surface->egl_display(), m_surface->egl_surface(), m_surface->egl_surface(),
                       m_surface->egl_context());
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

    size_t WaylandWindow::add_timer(int interval_ms, std::function<void()> callback, bool repeat)
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        Timer t;
        t.interval_ms = interval_ms;
        t.repeat = repeat;
        t.callback = std::move(callback);
        t.next_expiry = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count() +
                        interval_ms;
        size_t id = m_next_timer_id++;
        t.id = id;
        m_timers[id] = t;
        return id;
    }

    void WaylandWindow::stop_timer(size_t id)
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
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

        m_all_widgets.erase(widget);

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

        if (m_popup_menu == (Menu *)widget)
            close_context_menu();

        if (m_clipboard_backend)
            m_clipboard_backend->on_widget_destroyed(widget);
    }

    void WaylandWindow::register_widget(Widget *widget)
    {
        if (widget)
            m_all_widgets.insert(widget);
    }

    bool WaylandWindow::is_widget_alive(Widget *widget) const
    {
        return m_all_widgets.find(widget) != m_all_widgets.end();
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

    void WaylandWindow::set_focused_widget(Widget *widget)
    {
        if (m_focused == widget)
            return;

        if (m_focused)
        {
            m_focused->set_focus(false);
        }

        m_focused = widget;

        if (m_focused)
        {
            m_focused->set_focus(true);
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

    void WaylandWindow::show_context_menu(Menu *menu, int x, int y, uint32_t serial, Widget *owner)
    {
        close_context_menu(false);

        if (!m_surface || !menu)
        {
            LOG_ERROR << "[WINDOW] show_context_menu: surface or menu is NULL";
            return;
        }

        if (owner && owner->is_focusable())
        {
            owner->set_focus(true);
        }

        // Automatic Clipboard injection
        Widget *clipboard_target = owner;
        while (clipboard_target && !clipboard_target->supports_clipboard())
        {
            clipboard_target = clipboard_target->parent();
        }

        if (clipboard_target && clipboard_target->supports_clipboard())
        {
            // Check if it already has items
            bool has_clipboard = false;
            for (auto const &child : menu->children())
            {
                if (auto *item = dynamic_cast<MenuItem *>(child.get()))
                {
                    if (item->id() == "copy" || item->id() == "cut" || item->id() == "paste")
                    {
                        has_clipboard = true;
                        break;
                    }
                }
            }

            if (!has_clipboard)
            {
                if (!menu->children().empty())
                {
                    menu->add_separator();
                }

                auto *cut = menu->add_item("Cortar", "Ctrl+X", "clipboard_cut");
                auto *copy = menu->add_item("Copiar", "Ctrl+C", "clipboard_copy");
                auto *paste = menu->add_item("Pegar", "Ctrl+V", "clipboard_paste");

                cut->set_id("cut");
                copy->set_id("copy");
                paste->set_id("paste");

                cut->set_emit_signal_manager(false);
                copy->set_emit_signal_manager(false);
                paste->set_emit_signal_manager(false);

                cut->when_click.connect([clipboard_target](auto &)
                                        { clipboard_target->perform(ClipboardAction::Cut); });
                copy->when_click.connect([clipboard_target](auto &)
                                         { clipboard_target->perform(ClipboardAction::Copy); });
                paste->when_click.connect([clipboard_target](auto &)
                                          { clipboard_target->perform(ClipboardAction::Paste); });

                cut->set_enabled(clipboard_target->can_perform(ClipboardAction::Cut));
                copy->set_enabled(clipboard_target->can_perform(ClipboardAction::Copy));
                paste->set_enabled(clipboard_target->can_perform(ClipboardAction::Paste));
            }
        }

        // Automatic Fullscreen injection
        if (owner && owner->supports_fullscreen())
        {
            // Check if it already has it
            bool has_fullscreen = false;
            for (auto const &child : menu->children())
            {
                if (auto *item = dynamic_cast<MenuItem *>(child.get()))
                {
                    if (item->id() == "fullscreen_context")
                    {
                        has_fullscreen = true;
                        break;
                    }
                }
            }

            if (!has_fullscreen)
            {
                if (!menu->children().empty())
                {
                    menu->add_separator();
                }

                auto *item = menu->add_item(
                    is_fullscreen() ? "Salir de pantalla completa" : "Pantalla completa", "F11");
                item->set_id("fullscreen_context");
                item->when_click.connect(
                    [target_window = this](MouseButtonEventContext &)
                    {
                        if (target_window->is_fullscreen())
                            target_window->unfullscreen();
                        else
                            target_window->fullscreen();
                    });
            }
        }

        close_context_menu(false);

        if (x == -1 && y == -1)
        {
            x = (int)m_pointer_x;
            y = (int)m_pointer_y;
        }

        m_popup_menu = menu;
        m_popup_menu->set_application_recursive(this);
        m_popup_menu->set_visible(true);
        m_popup_menu->set_position(0, 0);

        // Determine the height of the monitor the window is currently on
        int monitor_h = m_surface->monitor_height();
        if (monitor_h <= 0)
        {
            // Fallback: Use the first monitor from SystemInfo if Wayland hasn't reported one yet
            auto monitors = SystemInfo::get_monitors();
            if (!monitors.empty())
                monitor_h = monitors[0].height;
            else
                monitor_h = 1080; // Safe default
        }

        // Cap the menu height to 80% of the active monitor's height
        m_popup_menu->set_max_menu_height((int)(monitor_h * 0.8));

        m_popup_menu->calculate_layout();

        int w = m_popup_menu->width();
        int h = m_popup_menu->height();

        // Calculate total surface size needed dynamically based on the submenu tree.
        // cascade_w accounts for all nested submenus at any depth.
        int cascade_w = m_popup_menu->calculate_cascade_width();
        if (cascade_w == 0) cascade_w = w; // Ensure at least one extra submenu can fit
        int surface_w = w + cascade_w + 20; // +20 margin
        int surface_h = std::max(h, monitor_h - y - 20);

        m_popup_surface = std::make_unique<WaylandSurface>(surface_w, surface_h);

        m_popup_listener = std::make_unique<PopupEventListener>(this, serial);
        m_popup_surface->set_event_listener(m_popup_listener.get());

        if (serial > 0)
        {
            m_surface->set_last_serial(serial);
        }

        // Pass real menu dimensions (w, h) as popup_w/popup_h.
        // setup_xdg_popup will call xdg_surface_set_window_geometry(0,0,w,h) BEFORE
        // the first commit, so the compositor anchors at the cursor correctly.
        m_popup_surface->setup_xdg_popup(m_surface.get(), x, y, surface_w, surface_h, w, h);

        invalidate();
    }

    void WaylandWindow::hide_context_menu()
    {
        if (m_popup_menu)
        {
            m_popup_menu->set_visible(false);
        }
        invalidate();
    }

    void WaylandWindow::close_context_menu(bool emit_signal, uint32_t serial)
    {
        if (m_popup_menu)
        {
            m_popup_menu->set_visible(false);
            // Move the menu to a temporary variable and destroy it in a deferred task
            // This prevents segfaults if a menu item action blocks the thread (e.g., opens a dialog)
            auto menu_to_destroy = m_popup_menu;
            m_popup_menu = nullptr;
            post_task([menu = menu_to_destroy]() {
                // The menu will be destroyed when this lambda goes out of scope, safely outside the click handler
            });
        }

        if (m_popup_surface || m_popup_listener)
        {
            m_popup_surface = nullptr;
        
        // Force the compositor to process the destruction immediately
        if (m_surface && m_surface->display()) {
            wl_display_roundtrip(m_surface->display());
        }
            if (m_popup_listener)
            {
                m_popup_listener->deactivate();
            }
            m_popup_listener = nullptr;
        }
        invalidate();

        if (emit_signal)
        {
            post_task([this, serial]() {
                PopupDismissedContext ctx;
                ctx.serial = serial;
                when_popup_dismissed.run(ctx);
            });
        }
    }

    void WaylandWindow::show_vault(Vault *vault, int x, int y, uint32_t serial, Widget *owner)
    {
        close_vault(false);
        close_context_menu(false);

        if (!m_surface || !vault)
        {
            LOG_ERROR << "[WINDOW] show_vault: surface or vault is NULL";
            return;
        }

        m_popup_vault = vault;
        m_popup_vault->set_application_recursive(this);
        m_popup_vault->set_visible(true);
        m_popup_vault->calculate_layout();

        int monitor_w = m_surface->monitor_width();
        int monitor_h = m_surface->monitor_height();
        if (monitor_w <= 0 || monitor_h <= 0)
        {
            auto monitors = SystemInfo::get_monitors();
            if (!monitors.empty()) {
                monitor_w = monitors[0].width;
                monitor_h = monitors[0].height;
            } else {
                monitor_w = 1920;
                monitor_h = 1080;
            }
        }

        int w = m_popup_vault->width();
        int h = m_popup_vault->height();

        if (owner && x == -1 && y == -1)
        {
            int ox = owner->x();
            int oy = owner->y();
            int ow = owner->width();
            int oh = owner->height();

            // Check if it's on the left/right/top/bottom edge
            if (ox < 100) { // Left toolbar
                x = ox + ow + 4;
                y = oy + (oh / 2) - (h / 2);
                if (y < 0) y = 4;
                if (y + h > monitor_h) y = monitor_h - h - 4;
                m_popup_vault->set_arrow_position(0, (oy + oh / 2) - y);
            } else if (ox > monitor_w - 100) { // Right toolbar
                x = ox - w - 4;
                y = oy + (oh / 2) - (h / 2);
                if (y < 0) y = 4;
                if (y + h > monitor_h) y = monitor_h - h - 4;
                m_popup_vault->set_arrow_position(w, (oy + oh / 2) - y);
            } else if (oy < 100) { // Top toolbar
                y = oy + oh + 4;
                x = ox + (ow / 2) - (w / 2);
                if (x < 0) x = 4;
                if (x + w > monitor_w) x = monitor_w - w - 4;
                m_popup_vault->set_arrow_position((ox + ow / 2) - x, 0);
            } else { // Bottom toolbar
                y = oy - h - 4;
                x = ox + (ow / 2) - (w / 2);
                if (x < 0) x = 4;
                if (x + w > monitor_w) x = monitor_w - w - 4;
                m_popup_vault->set_arrow_position((ox + ow / 2) - x, h);
            }
        }
        else if (x == -1 && y == -1)
        {
            x = (int)m_pointer_x;
            y = (int)m_pointer_y;
            m_popup_vault->set_arrow_position(-1, -1);
        }

        // Position vault at origin of the popup surface - no offset
        m_popup_vault->set_position(0, 0);
        m_popup_vault->calculate_layout();

        // Surface exactly matches Vault size
        int surface_w = m_popup_vault->width();
        int surface_h = m_popup_vault->height();

        m_popup_surface = std::make_unique<WaylandSurface>(surface_w, surface_h);
        m_popup_surface->set_blur(true);
        m_popup_listener = std::make_unique<PopupEventListener>(this, serial);
        m_popup_surface->set_event_listener(m_popup_listener.get());

        if (serial > 0)
        {
            m_surface->set_last_serial(serial);
        }

        m_popup_surface->setup_xdg_popup(m_surface.get(), x, y, surface_w, surface_h, w, h, false); // No grab: Vault needs to be interactive

        invalidate();
    }

    void WaylandWindow::hide_vault()
    {
        if (m_popup_vault)
        {
            m_popup_vault->set_visible(false);
        }
        invalidate();
    }

    void WaylandWindow::close_vault(bool emit_signal, uint32_t serial)
    {
        if (m_popup_vault)
        {
            m_popup_vault->set_visible(false);
            m_popup_vault = nullptr;
        }

        if (m_popup_surface || m_popup_listener)
        {
            m_popup_surface = nullptr;
            if (m_surface && m_surface->display()) {
                wl_display_roundtrip(m_surface->display());
            }
            if (m_popup_listener)
            {
                m_popup_listener->deactivate();
            }
            m_popup_listener = nullptr;
        }
        invalidate();

        if (emit_signal)
        {
            post_task([this, serial]() {
                PopupDismissedContext ctx;
                ctx.serial = serial;
                when_popup_dismissed.run(ctx);
            });
        }
    }

    widget_position WaylandWindow::get_global_pointer_position() const
    {
        return {m_screen_x + (int)m_pointer_x, m_screen_y + (int)m_pointer_y};
    }

    void WaylandWindow::show_tooltip(Widget *owner, Notification *tooltip)
    {
        if (!owner || !tooltip || m_tooltip_owner == owner)
            return;

        hide_tooltip();

        m_tooltip_owner = owner;
        m_tooltip_widget = tooltip;
        m_tooltip_widget->set_application_recursive(this);
        m_tooltip_widget->set_visible(true);
        m_tooltip_widget->set_position(0, 0);

        // Calculate layout with a maximum width to allow wrapping
        int max_w = 400; // Tooltip max width
        int h = m_tooltip_widget->preferred_height(max_w);
        int w = m_tooltip_widget->preferred_width();
        if (w > max_w)
            w = max_w;

        m_tooltip_widget->set_size(w, h);
        m_tooltip_widget->calculate_layout();

        m_tooltip_surface = std::make_unique<WaylandSurface>(w, h);
        m_tooltip_surface->share_connection_from(m_surface.get());

        // Tooltip position: 20px below the mouse cursor
        int x = (int)m_pointer_x;
        int y = (int)m_pointer_y + 20;

        m_tooltip_surface->setup_xdg_popup(m_surface.get(), x, y, w, h);
        invalidate();
    }

    void WaylandWindow::hide_tooltip()
    {
        if (m_tooltip_surface || m_tooltip_widget)
        {
            m_tooltip_surface = nullptr;
            m_tooltip_widget = nullptr;
            m_tooltip_owner = nullptr;
            invalidate();
        }
    }

    void WaylandWindow::request_clipboard_data(Widget *target, const std::string &mime_type)
    {
        if (m_clipboard_backend)
        {
            auto target_sink = std::make_shared<WidgetDataSink>(target, mime_type);
            auto loopback_sink = std::make_shared<MainThreadDataSink>(this, target_sink);

            m_clipboard_backend->request_data(mime_type, loopback_sink);
        }
    }

    void WaylandWindow::set_clipboard_owner(Widget *owner)
    {
        if (m_clipboard_backend && owner)
        {
            m_clipboard_backend->set_provider(owner->get_clipboard_provider(),
                                              owner->provided_mime_types());
        }
    }

    Widget *WaylandWindow::find_clipboard_target()
    {
        // 1. Bottom-up search starting from the focused widget
        if (m_focused)
        {
            LOG_INFO
                << "WaylandWindow: find_clipboard_target starting bottom-up from focused widget";
            Widget *temp = m_focused;
            while (temp)
            {
                if (temp->supports_clipboard())
                {
                    LOG_INFO
                        << "WaylandWindow: find_clipboard_target found candidate in parent chain";
                    return temp;
                }
                temp = temp->parent();
            }
        }

        // 2. Global fallback search (top-down)
        if (m_root)
        {
            LOG_INFO
                << "WaylandWindow: find_clipboard_target falling back to top-down search from root";
            auto *found = detect_clipboard_target(m_root.get());
            if (found)
                LOG_INFO
                    << "WaylandWindow: find_clipboard_target found candidate via top-down search";
            return found;
        }

        LOG_INFO << "WaylandWindow: find_clipboard_target returned NULL";
        return nullptr;
    }

    bool WaylandWindow::detect_undo_support(Widget *root)
    {
        if (!root)
            return false;
        if (root->supports_undo())
            return true;
        for (const auto &child : root->children())
        {
            if (detect_undo_support(child.get()))
                return true;
        }
        return false;
    }

    Widget *WaylandWindow::find_undo_target()
    {
        if (m_focused)
        {
            Widget *temp = m_focused;
            while (temp)
            {
                if (temp->supports_undo())
                    return temp;
                temp = temp->parent();
            }
        }

        std::function<Widget*(Widget*)> search_top_down = [&](Widget *w) -> Widget* {
            if (!w) return nullptr;
            if (w->supports_undo()) return w;
            for (const auto &child : w->children()) {
                if (auto *found = search_top_down(child.get())) return found;
            }
            return nullptr;
        };

        if (m_root)
        {
            return search_top_down(m_root.get());
        }

        return nullptr;
    }

    bool WaylandWindow::detect_zoom_support(Widget *root)
    {
        if (!root)
            return false;
        if (root->supports_zoom())
            return true;
        for (const auto &child : root->children())
        {
            if (detect_zoom_support(child.get()))
                return true;
        }
        return false;
    }

    Widget *WaylandWindow::find_zoom_target()
    {
        if (m_focused)
        {
            Widget *temp = m_focused;
            while (temp)
            {
                if (temp->supports_zoom())
                    return temp;
                temp = temp->parent();
            }
        }

        std::function<Widget*(Widget*)> search_top_down = [&](Widget *w) -> Widget* {
            if (!w) return nullptr;
            if (w->supports_zoom()) return w;
            for (const auto &child : w->children()) {
                if (auto *found = search_top_down(child.get())) return found;
            }
            return nullptr;
        };

        if (m_root)
        {
            return search_top_down(m_root.get());
        }

        return nullptr;
    }

    bool WaylandWindow::detect_print_support(Widget *root)
    {
        if (!root)
            return false;
        if (root->supports_printing())
            return true;
        for (const auto &child : root->children())
        {
            if (detect_print_support(child.get()))
                return true;
        }
        return false;
    }

    Widget *WaylandWindow::find_print_target()
    {
        if (m_focused)
        {
            Widget *temp = m_focused;
            while (temp)
            {
                if (temp->supports_printing())
                    return temp;
                temp = temp->parent();
            }
        }

        std::function<Widget*(Widget*)> search_top_down = [&](Widget *w) -> Widget* {
            if (!w) return nullptr;
            if (w->supports_printing()) return w;
            for (const auto &child : w->children()) {
                if (auto *found = search_top_down(child.get())) return found;
            }
            return nullptr;
        };

        if (m_root)
        {
            return search_top_down(m_root.get());
        }

        return nullptr;
    }


    std::vector<std::string> WaylandWindow::get_clipboard_mime_types() const
    {
        if (m_clipboard_backend)
        {
            return m_clipboard_backend->get_mime_types();
        }
        return {};
    }

    void WaylandWindow::apply_fullscreen_isolation(Widget *target)
    {
        if (!target || !m_root)
            return;

        m_fullscreen_target = target;
        m_hidden_by_fullscreen.clear();

        // 1. Identify valid path (target and its ancestors)
        std::vector<Widget *> valid_path;
        Widget *temp = target;
        while (temp)
        {
            valid_path.push_back(temp);
            temp = temp->parent();
        }

        // 2. Recursive hiding of siblings
        std::function<void(Widget *)> traverse = [&](Widget *w)
        {
            for (auto const &child : w->children())
            {
                // If child is NOT in the valid path, hide it and its subtree
                auto it = std::find(valid_path.begin(), valid_path.end(), child.get());
                if (it == valid_path.end())
                {
                    if (child->is_visible())
                    {
                        child->set_visible(false);
                        m_hidden_by_fullscreen.push_back(child.get());
                    }
                }
                else
                {
                    // If child IS in the valid path, we need to visit its children
                    // EXCEPT if it's the target itself (all descendants of the target
                    // should remain visible).
                    if (child.get() != target)
                    {
                        traverse(child.get());
                    }
                }
            }
        };

        traverse(m_root.get());
    }

    void WaylandWindow::restore_fullscreen_isolation()
    {
        for (Widget *w : m_hidden_by_fullscreen)
        {
            w->set_visible(true);
        }
        m_hidden_by_fullscreen.clear();
        m_fullscreen_target = nullptr;
    }

    bool WaylandWindow::detect_fullscreen_support(Widget *root)
    {
        if (!root)
            return false;

        if (root->supports_fullscreen())
            return true;

        for (auto const &child : root->children())
        {
            if (detect_fullscreen_support(child.get()))
                return true;
        }

        return false;
    }

    Widget *WaylandWindow::find_fullscreen_target(Widget *root)
    {
        if (!root || !root->is_visible())
            return nullptr;

        if (root->supports_fullscreen())
            return root;

        for (auto const &child : root->children())
        {
            Widget *target = find_fullscreen_target(child.get());
            if (target)
                return target;
        }

        return nullptr;
    }

    Window *WaylandWindow::find_window_target(Widget *root)
    {
        if (!root || !root->is_visible())
            return nullptr;

        if (Window *win = dynamic_cast<Window *>(root))
        {
            if (win->file_capabilities() != FileNone)
                return win;
        }

        for (auto &child : root->children())
        {
            if (Window *win = find_window_target(child.get()))
                return win;
        }
        return nullptr;
    }

    void WaylandWindow::alert(const std::string &message, const std::string &title,
                              MessageType type)
    {
        auto dialog = std::make_unique<MessageDialog>(title, message, type, false);
        std::thread(
            [d = std::move(dialog)]() mutable
            {
                d->initialize();
                d->run();
            })
            .detach();
    }

    bool WaylandWindow::confirm(const std::string &message, const std::string &title,
                                MessageType type)
    {
        auto dialog = std::make_unique<MessageDialog>(title, message, type, true);
        std::promise<bool> promise;
        auto future = promise.get_future();

        dialog->when_responded.connect(
            [&promise](MessageResponseEvent res)
            { promise.set_value(res.response == MessageResponse::Accept); });

        std::thread(
            [d = std::move(dialog)]() mutable
            {
                d->initialize();
                d->run();
            })
            .detach();

        return future.get();
    }

    void WaylandWindow::set_preferences_content(PreferencesFactory factory, int width, int height)
    {
        m_preferences_factory = std::move(factory);
        m_preferences_width = width;
        m_preferences_height = height;
        if (m_is_running)
        {
            init_global_menu(); // Refresh menu if already running
        }
    }

    void WaylandWindow::show_preferences()
    {
        if (!m_preferences_factory)
        {
            LOG_ERROR << "[WINDOW] show_preferences: no PreferencesFactory set";
            return;
        }

        // Invoking the factory to create a fresh PreferencesContent for this dialog
        auto content = m_preferences_factory();

        if (!content)
        {
            LOG_ERROR << "[WINDOW] show_preferences: factory returned null content";
            return;
        }

        // We run the dialog in a separate thread.
        std::thread(
            [this, content = std::move(content)]() mutable
            {
                auto dialog = std::make_unique<DialogPreferences>(
                    i18n().tr("core.global_menu.preferences"), m_preferences_width,
                    m_preferences_height, true);

                // Setup the toolbar automatically using the PreferencesContent
                dialog->setup_toolbar(content.get());
                dialog->set_content(std::move(content));

                dialog->initialize();
                dialog->run();
            })
            .detach();
    }


    void WaylandWindow::show_aboutus()
    {
        if (m_about_manager)
        {
            show_about_dialog(*m_about_manager);
        }
        else
        {
            LOG_WARNING << "[WINDOW] show_aboutus(legacy) called. Please migrate to show_about_dialog(AboutManager).";
        }
    }

    void WaylandWindow::set_about_manager(AboutManager *manager)
    {
        m_about_manager = manager;
    }

    void WaylandWindow::show_about_dialog(AboutManager &manager)
    {
        std::thread(
            [&manager]()
            {
                auto dialog = std::make_unique<AboutUsDialog>(manager);
                dialog->show();
            })
            .detach();
    }

    void WaylandWindow::set_use_global_menu(bool use)
    {
        m_use_global_menu = use;
    }
    void WaylandWindow::focus_next()
    {
        std::vector<Widget *> focusables;
        collect_focusable_widgets(m_root.get(), focusables);

        if (focusables.empty())
            return;

        int next_index = 0;
        if (m_focused)
        {
            auto it = std::find(focusables.begin(), focusables.end(), m_focused);
            if (it != focusables.end())
            {
                next_index = (std::distance(focusables.begin(), it) + 1) % focusables.size();
            }
        }

        set_focused_widget(focusables[next_index]);
    }

    void WaylandWindow::focus_previous()
    {
        std::vector<Widget *> focusables;
        collect_focusable_widgets(m_root.get(), focusables);

        if (focusables.empty())
            return;

        int prev_index = (int)focusables.size() - 1;
        if (m_focused)
        {
            auto it = std::find(focusables.begin(), focusables.end(), m_focused);
            if (it != focusables.end())
            {
                int current_index = (int)std::distance(focusables.begin(), it);
                prev_index = (current_index - 1 + (int)focusables.size()) % (int)focusables.size();
            }
        }

        set_focused_widget(focusables[prev_index]);
    }

    void WaylandWindow::collect_focusable_widgets(Widget *root, std::vector<Widget *> &list)
    {
        if (!root || !root->is_visible() || !root->is_enabled())
            return;

        if (root->is_focusable())
        {
            list.push_back(root);
        }

        for (auto const &child : root->children())
        {
            collect_focusable_widgets(child.get(), list);
        }
    }

    struct DragSourceState
    {
        WaylandWindow *window;
        std::function<std::vector<uint8_t>(const std::string &)> fetcher;
    };

    static void data_source_handle_send(void *data, struct wl_data_source *source, const char *mime_type, int fd)
    {
        auto *state = static_cast<DragSourceState *>(data);
        if (state->fetcher)
        {
            auto bytes = state->fetcher(mime_type ? mime_type : "");
            write(fd, bytes.data(), bytes.size());
        }
        close(fd);
    }

    static void data_source_handle_cancelled(void *data, struct wl_data_source *source)
    {
        auto *state = static_cast<DragSourceState *>(data);
        if (state->window && state->window->w_surface())
        {
            state->window->w_surface()->set_cursor(CursorType::Default);
        }
        state->window->cleanup_drag_icon();
        delete state;
        wl_data_source_destroy(source);
    }

    static void data_source_handle_dnd_finished(void *data, struct wl_data_source *source)
    {
        auto *state = static_cast<DragSourceState *>(data);
        if (state->window && state->window->w_surface())
        {
            state->window->w_surface()->set_cursor(CursorType::Default);
        }
        state->window->cleanup_drag_icon();
        delete state;
        wl_data_source_destroy(source);
    }

    static const struct wl_data_source_listener data_source_listener = {
        .target = [](void *data, struct wl_data_source *source, const char *mime_type) {
            auto *state = static_cast<DragSourceState *>(data);
            if (state->window && state->window->w_surface())
            {
                if (mime_type)
                {
                    state->window->w_surface()->set_cursor(CursorType::DndCopy);
                }
                else
                {
                    state->window->w_surface()->set_cursor(CursorType::DndNone);
                }
            }
        },
        .send = data_source_handle_send,
        .cancelled = data_source_handle_cancelled,
        .dnd_drop_performed = [](void *, struct wl_data_source *) {},
        .dnd_finished = data_source_handle_dnd_finished,
        .action = [](void *data, struct wl_data_source *source, uint32_t action) {
             auto *state = static_cast<DragSourceState *>(data);
             if (state->window && state->window->w_surface())
             {
                 if (action == WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY)
                     state->window->w_surface()->set_cursor(CursorType::DndCopy);
                 else if (action == WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE)
                     state->window->w_surface()->set_cursor(CursorType::DndMove);
                 else if (action == WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE)
                     state->window->w_surface()->set_cursor(CursorType::DndNone);
             }
        },
    };

    WaylandWindow* WaylandWindow::s_active_drag_source = nullptr;

    void WaylandWindow::start_drag(const std::vector<std::string> &mime_types,
                                   std::function<std::vector<uint8_t>(const std::string &)> data_fetcher,
                                   Widget *icon_widget)
    {
        if (!m_surface->data_device_manager() || !m_surface->data_device())
            return;

        struct wl_data_source *source =
            wl_data_device_manager_create_data_source(m_surface->data_device_manager());

        for (const auto &mime : mime_types)
        {
            wl_data_source_offer(source, mime.c_str());
        }

        auto *state = new DragSourceState{this, data_fetcher};
        m_current_fetcher = data_fetcher;
        s_active_drag_source = this;
        wl_data_source_add_listener(source, &data_source_listener, state);

        struct wl_surface *icon_surf = nullptr;
        if (icon_widget)
        {
            // Create a small surface for the icon
            int iw = icon_widget->width() > 0 ? icon_widget->width() : 64;
            int ih = icon_widget->height() > 0 ? icon_widget->height() : 64;
            
            m_drag_icon_surface = std::make_unique<WaylandSurface>(iw, ih);
            m_drag_icon_surface->share_connection_from(m_surface.get());
            m_drag_icon_surface->setup_drag_icon();
            
            // For now, we'll just clear it with a semi-transparent color as a placeholder
            // because full widget rendering into a sub-surface requires a more complex GL state management.
            // But we will commit it so it's visible.
            m_drag_icon_surface->resize_buffer(iw, ih);
            icon_surf = m_drag_icon_surface->surface();
            wl_surface_commit(icon_surf);
        }

        if (wl_proxy_get_version((struct wl_proxy *)source) >= 3)
        {
            wl_data_source_set_actions(source, 
                WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY | WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);
        }

        wl_data_device_start_drag(m_surface->data_device(), source, m_surface->surface(), icon_surf,
                                  m_last_serial);
    }

    void WaylandWindow::cleanup_drag_icon()
    {
        m_drag_icon_surface.reset();
        s_active_drag_source = nullptr;
    }

} // namespace horizon
