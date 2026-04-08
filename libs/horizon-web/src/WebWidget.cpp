#include "horizon/web/WebWidget.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/Logger.hpp"
#include "horizon/WaylandWindow.hpp"
#include <cstring>
#include <wayland-server-core.h>
#include <wpe/fdo.h>
#include <wpe/unstable/fdo-shm.h>
#include <wpe/webkit.h>
#include <jsc/jsc.h>
#include <glib.h>

namespace horizon
{
    namespace web
    {
        // Helper to map Horizon modifiers to WPE modifiers
        static uint32_t map_horizon_to_wpe_modifiers(uint32_t mods)
        {
            uint32_t result = 0;
            if (mods & 0x1) result |= (1 << 1); // Shift (Horizon 0x1 -> WPE Shift 1<<1)
            if (mods & 0x2) result |= (1 << 0); // Control (Horizon 0x2 -> WPE Control 1<<0)
            if (mods & 0x4) result |= (1 << 2); // Alt (Horizon 0x4 -> WPE Alt 1<<2)
            return result;
        }

        static uint32_t map_to_wpe_button(uint32_t button)
        {
            switch (button)
            {
            case 272: return 1; // BTN_LEFT
            case 273: return 2; // BTN_RIGHT
            case 274: return 3; // BTN_MIDDLE
            default: return button;
            }
        }

        std::thread WebWidget::s_worker_thread;
        GMainContext* WebWidget::s_worker_context = nullptr;
        GMainLoop* WebWidget::s_worker_loop = nullptr;
        bool WebWidget::s_running = false;
        std::mutex WebWidget::s_worker_mutex;

        static int s_instance_count = 0;
        static std::mutex s_instance_mutex;

        WebWidget::WebWidget()
        {
            set_focusable(true);
            // Thumbs are drawn directly now

            // Initialization is now managed by a shared static method called from the worker thread

            // Mouse Press
            when_mouse_press.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    // Check for scrollbar interaction first
                    if (m_show_v_scroll && ctx.x >= m_v_track_x && ctx.x <= m_v_track_x + m_v_track_w &&
                        ctx.y >= m_v_track_y && ctx.y <= m_v_track_y + m_v_track_h) {
                        m_is_dragging_v = true;
                        m_drag_start_pos = ctx.y;
                        {
                            std::lock_guard<std::mutex> lock{m_scroll_mutex};
                            m_drag_start_scroll = m_scroll_y;
                        }
                        return;
                    }
                    if (m_show_h_scroll && ctx.x >= m_h_track_x && ctx.x <= m_h_track_x + m_h_track_w &&
                        ctx.y >= m_h_track_y && ctx.y <= m_h_track_y + m_h_track_h) {
                        m_is_dragging_h = true;
                        m_drag_start_pos = ctx.x;
                        {
                            std::lock_guard<std::mutex> lock{m_scroll_mutex};
                            m_drag_start_scroll = m_scroll_x;
                        }
                        return;
                    }

                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_button,
                                                             0, 
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             map_to_wpe_button(ctx.button),
                                                             1,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers)};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebWidget*, wpe_input_pointer_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_pointer_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebWidget*, wpe_input_pointer_event*>(this, event));
                });

            // Mouse Release
            when_mouse_release.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    m_is_dragging_v = false;
                    m_is_dragging_h = false;

                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_button,
                                                             0,
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             map_to_wpe_button(ctx.button),
                                                             0,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers)};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebWidget*, wpe_input_pointer_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_pointer_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebWidget*, wpe_input_pointer_event*>(this, event));
                });

            // Mouse Move
            when_mouse_move.connect(
                [this](MouseMoveEventContext &ctx)
                {
                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_motion,
                                                             0,
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             0,
                                                             0,
                                                             0};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebWidget*, wpe_input_pointer_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_pointer_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebWidget*, wpe_input_pointer_event*>(this, event));
                });

            // Mouse Drag
            when_mouse_drag.connect(
                [this](MouseMoveEventContext &ctx)
                {
                    if (m_is_dragging_v) {
                        int delta = ctx.y - m_drag_start_pos;
                        double scrollable_height = m_content_height - height();
                        double track_space = m_v_track_h - 20;
                        if (track_space > 0) {
                            double scroll_delta = (double)delta / track_space * scrollable_height;
                            handle_ui_scroll(-1, (int)(m_drag_start_scroll + scroll_delta));
                        }
                    } else if (m_is_dragging_h) {
                        int delta = ctx.x - m_drag_start_pos;
                        double scrollable_width = m_content_width - width();
                        double track_space = m_h_track_w - 20;
                        if (track_space > 0) {
                            double scroll_delta = (double)delta / track_space * scrollable_width;
                            handle_ui_scroll((int)(m_drag_start_scroll + scroll_delta), -1);
                        }
                    }
                });

            // Mouse Wheel
            when_mouse_wheel.connect(
                [this](MouseWheelEventContext &ctx)
                {
                    uint32_t wpe_mods = map_horizon_to_wpe_modifiers(ctx.modifiers);

                    auto dispatch_axis = [this, &ctx, wpe_mods](uint32_t axis, int32_t value) {
                        if (value == 0) return;
                        
                        auto* event = new wpe_input_axis_event{
                            wpe_input_axis_event_type_motion,
                            0u,
                            (int)(ctx.x - x()),
                            (int)(ctx.y - y()),
                            axis,
                            value,
                            wpe_mods};

                        g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                            auto* d = static_cast<std::pair<WebWidget*, wpe_input_axis_event*>*>(data);
                            if (d->first->m_backend) wpe_view_backend_dispatch_axis_event(d->first->m_backend, d->second);
                            delete d->second;
                            delete d;
                            return FALSE;
                        }, new std::pair<WebWidget*, wpe_input_axis_event*>(this, event));
                    };

                    // Scale factor: dy/dx in Wayland are typically ~10 units per notch.
                    // WebKit often expects these to be roughly pixel-equivalent or at least significant.
                    // Positive multiplier because WebKit expects positive value for scrolling DOWN.
                    dispatch_axis(0, (int)(ctx.dy * 8)); // Vertical
                    dispatch_axis(1, (int)(ctx.dx * 8)); // Horizontal
                });

            // Keyboard
            when_key_press.connect(
                [this](KeyEventContext &ctx)
                {
                    auto* event = new wpe_input_keyboard_event{0, ctx.keysym, ctx.key + 8, true,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers)};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebWidget*, wpe_input_keyboard_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_keyboard_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebWidget*, wpe_input_keyboard_event*>(this, event));
                });

            when_key_release.connect(
                [this](KeyEventContext &ctx)
                {
                    auto* event = new wpe_input_keyboard_event{0, ctx.keysym, ctx.key + 8, false,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers)};

                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebWidget*, wpe_input_keyboard_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_keyboard_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebWidget*, wpe_input_keyboard_event*>(this, event));
                });

            // Interaction for Scrollbars
            when_mouse_press.connect([this](MouseButtonEventContext &ev) {
                if (m_show_v_scroll && ev.x >= m_v_track_x && ev.x < m_v_track_x + m_v_track_w &&
                    ev.y >= m_v_track_y && ev.y < m_v_track_y + m_v_track_h) {
                    m_is_dragging_v = true;
                    m_drag_start_pos = ev.y;
                    m_drag_start_scroll = m_scroll_y;
                    ev.stop_propagation = true;
                } else if (m_show_h_scroll && ev.y >= m_h_track_y && ev.y < m_h_track_y + m_h_track_h &&
                         ev.x >= m_h_track_x && ev.x < m_h_track_x + m_h_track_w) {
                    m_is_dragging_h = true;
                    m_drag_start_pos = ev.x;
                    m_drag_start_scroll = m_scroll_x;
                    ev.stop_propagation = true;
                }
            });

            when_mouse_drag.connect([this](MouseMoveEventContext &ev) {
                if (m_is_dragging_v) {
                    double delta_y = ev.y - m_drag_start_pos;
                    double track_usable = m_v_track_h - std::max(20, (int)(m_v_track_h * ((double)height() / m_content_height)));
                    if (track_usable > 0) {
                        double scroll_max = m_content_height - height();
                        double new_y = m_drag_start_scroll + (delta_y * (scroll_max / track_usable));
                        handle_ui_scroll(-1, (int)new_y);
                    }
                } else if (m_is_dragging_h) {
                    double delta_x = ev.x - m_drag_start_pos;
                    double track_usable = m_h_track_w - std::max(20, (int)(m_h_track_w * ((double)width() / m_content_width)));
                    if (track_usable > 0) {
                        double scroll_max = m_content_width - width();
                        double new_x = m_drag_start_scroll + (delta_x * (scroll_max / track_usable));
                        handle_ui_scroll((int)new_x, -1);
                    }
                }
            });

            when_mouse_release.connect([this](MouseButtonEventContext &) {
                m_is_dragging_v = false;
                m_is_dragging_h = false;
            });

            m_last_v_show_time = std::chrono::steady_clock::now();
            m_last_h_show_time = std::chrono::steady_clock::now();
        }

        WebWidget::~WebWidget()
        {
            // Disconnect all signals immediately to prevent any worker callbacks to dead UI objects
            when_title_changed.disconnect_all();
            when_url_changed.disconnect_all();
            when_loading_changed.disconnect_all();
            when_progress_changed.disconnect_all();

            // Synchronous cleanup on worker thread
            struct CleanupData {
                WebKitWebView* web_view;
                wpe_view_backend_exportable_fdo* exportable;
                std::atomic<bool>* done;
            };
            
            std::atomic<bool> cleanup_done{false};

            {
                std::lock_guard<std::mutex> lock(s_worker_mutex);
                if (!s_running || !s_worker_context) {
                    // Global shutdown in progress. 
                    // SKIP explicit destruction to avoid wpe_view_backend_destroy crashes.
                    // The OS will reclaim all resources safely upon process exit.
                    m_web_view = nullptr;
                    m_exportable = nullptr;
                    m_backend = nullptr;
                    return;
                }
            }

            auto* cd = new CleanupData{m_web_view, m_exportable, &cleanup_done};
            m_web_view = nullptr;
            m_exportable = nullptr;
            m_backend = nullptr;

            g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                auto* d = static_cast<CleanupData*>(data);
                // 1. Unref the webview first
                if (d->web_view) {
                    g_object_unref(d->web_view);
                }
                // 2. We SKIP explicit destruction of the exportable backend.
                // Calling wpe_view_backend_exportable_fdo_destroy(d->exportable) 
                // is causing internal segmentation faults in libwpe at exit
                // due to race conditions in its internal threads (VBlankMonitor).
                // The OS will reclaim these resources safely.
                
                d->done->store(true);
                delete d;
                return FALSE;
            }, cd);

            // Wait for worker thread to finish cleanup
            while (!cleanup_done.load()) {
                std::this_thread::yield();
            }

            {
                std::lock_guard<std::mutex> lock(m_surface_mutex);
                if (m_cairo_surface)
                {
                    cairo_surface_destroy(m_cairo_surface);
                    m_cairo_surface = nullptr;
                }
            }

            std::lock_guard<std::mutex> ilock(s_worker_mutex);
            s_instance_count--;
            // We no longer join the thread in the destructor.
            // Joining is now handled explicitly in WebWidget::shutdown().
        }

        void WebWidget::init_wpe()
        {
            if (m_initialized)
                return;

            ensure_worker_thread();
            
            {
                std::lock_guard<std::mutex> ilock(s_worker_mutex);
                s_instance_count++;
            }

            // Create backend/webview in the worker thread
            g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                auto* self = static_cast<WebWidget*>(data);
                
                static struct wpe_view_backend_exportable_fdo_client client = {
                    .export_buffer_resource = nullptr,
                    .export_dmabuf_resource = nullptr,
                    .export_shm_buffer =
                        [](void *data, struct wpe_fdo_shm_exported_buffer *buffer)
                    {
                        auto *self = static_cast<WebWidget *>(data);
                        WebWidget::on_frame_exported(self, buffer);
                    },
                    ._wpe_reserved0 = nullptr,
                    ._wpe_reserved1 = nullptr};

                int initial_width = self->width() > 0 ? self->width() : 800;
                int initial_height = self->height() > 0 ? self->height() : 600;

                self->m_exportable = wpe_view_backend_exportable_fdo_create(&client, self, initial_width, initial_height);
                self->m_backend = wpe_view_backend_exportable_fdo_get_view_backend(self->m_exportable);

                auto *webkit_backend = webkit_web_view_backend_new(self->m_backend, nullptr, nullptr);

                // --- PERFORMANCE OPTIMIZATIONS ---
                WebKitSettings* settings = webkit_settings_new();
                webkit_settings_set_enable_page_cache(settings, TRUE);
                webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
                webkit_settings_set_enable_webgl(settings, TRUE);
                webkit_settings_set_javascript_can_open_windows_automatically(settings, TRUE);
                
                self->m_web_view = webkit_web_view_new(webkit_backend);
                webkit_web_view_set_settings(self->m_web_view, settings);
                g_object_unref(settings); // WebView takes a ref

                LOG_INFO << "WebView created with performance optimizations for WebWidget: " << self;

                g_signal_connect(self->m_web_view, "notify::title", G_CALLBACK(on_title_notify), self);
                g_signal_connect(self->m_web_view, "notify::uri", G_CALLBACK(on_uri_notify), self);
                g_signal_connect(self->m_web_view, "load-changed", G_CALLBACK(on_load_changed), self);
                g_signal_connect(self->m_web_view, "notify::estimated-load-progress", G_CALLBACK(on_progress_notify), self);
                g_signal_connect(self->m_web_view, "mouse-target-changed", G_CALLBACK(on_mouse_target_changed), self);
                
                WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(self->m_web_view);
                
                const char* script_source = 
                    "const inject = () => {"
                    "  if (!document.documentElement) return false;"
                    "  const style = document.createElement('style');"
                    "  style.textContent = '::-webkit-scrollbar { display: none !important; }';"
                    "  (document.head || document.documentElement).appendChild(style);"
                    "  document.documentElement.style.scrollbarWidth = \"none\";" // Firefox-style extra safety
                    "  let ticking = false;"
                    "  let lastY = -1, lastX = -1, lastH = -1, lastW = -1;"
                    "  const beaconPrefix = 'HORIZON_SCROLL:';"
                    "  const sendScroll = (force = false) => {"
                    "    if (ticking && !force) return;"
                    "    const doc = document.documentElement;"
                    "    if (!doc) return;"
                    "    const sy = Math.round(window.scrollY), sx = Math.round(window.scrollX), ch = Math.round(doc.scrollHeight), cw = Math.round(doc.scrollWidth);"
                    "    if (!force && lastY === sy && lastX === sx && lastH === ch && lastW === cw) return;"
                    "    lastY = sy; lastX = sx; lastH = ch; lastW = cw;"
                    "    ticking = true;"
                    "    const action = () => {"
                    "      const beacon = beaconPrefix + sy + ' ' + sx + ' ' + ch + ' ' + cw;"
                    "      const oldTitle = document.title;"
                    "      document.title = beacon;"
                    "      if (oldTitle !== beacon) setTimeout(() => { if (document.title.startsWith(beaconPrefix)) document.title = oldTitle; }, 10);"
                    "      ticking = false;"
                    "    };"
                    "    if (force) action(); else setTimeout(action, 16);"
                    "  };"
                    "  window._horizon_send_scroll = () => sendScroll(true);"
                    "  window.addEventListener('scroll', () => sendScroll(), {passive: true});"
                    "  window.addEventListener('resize', () => sendScroll(), {passive: true});"
                    "  const observer = new ResizeObserver(() => sendScroll());"
                    "  observer.observe(document.documentElement);"
                    "  sendScroll(true);"
                    "  let initCount = 0;"
                    "  const initInterval = setInterval(() => {"
                    "    sendScroll(true);"
                    "    if (++initCount > 20) clearInterval(initInterval);"
                    "  }, 250);"
                    "  return true;"
                    "};"
                    "inject();"
                    "window.addEventListener('DOMContentLoaded', inject);"
                    "window.addEventListener('load', inject);";

                WebKitUserScript* script = webkit_user_script_new(
                    script_source,
                    WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                    WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
                    NULL, NULL);
                webkit_user_content_manager_add_script(manager, script);
                webkit_user_script_unref(script);

                // EXTRA AGGRESSIVE: Inject style sheet at USER level
                WebKitUserStyleSheet* style = webkit_user_style_sheet_new(
                    "::-webkit-scrollbar { display: none !important; }",
                    WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                    WEBKIT_USER_STYLE_LEVEL_USER, NULL, NULL);
                webkit_user_content_manager_add_style_sheet(manager, style);
                webkit_user_style_sheet_unref(style);

                // ENGINE LEVEL: We'll rely on the high-priority style sheet for now
                // as set_overlay_scrolling_enabled might not be available in this WebKit version.

                return FALSE;
            }, this);

            m_initialized = true;
        }

        void WebWidget::shutdown() {
            std::lock_guard<std::mutex> lock(s_worker_mutex);
            if (!s_running) return;

            LOG_INFO << "Global WebWidget shutdown initiated...";
            
            if (s_worker_loop) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    g_main_loop_quit((GMainLoop*)data);
                    return FALSE;
                }, s_worker_loop);
            }

            if (s_worker_thread.joinable()) {
                s_worker_thread.join();
            }
            s_running = false;
            LOG_INFO << "Global WebWidget shutdown complete.";
        }

        void WebWidget::ensure_worker_thread()
        {
            std::lock_guard<std::mutex> lock(s_worker_mutex);
            if (s_running) return;

            s_running = true;
            s_worker_thread = std::thread(&WebWidget::worker_thread_func);
            
            // Wait for context to be initialized
            while (!s_worker_context) {
                std::this_thread::yield();
            }
        }

        void WebWidget::worker_thread_func() 
        {
            s_worker_context = g_main_context_new();
            g_main_context_push_thread_default(s_worker_context);
            s_worker_loop = g_main_loop_new(s_worker_context, FALSE);

            // WPE Core Init with detection
            LOG_INFO << "Detecting WPE Backend library...";
            const char* paths[] = {
                "/usr/lib/x86_64-linux-gnu/libWPEBackend-fdo-1.0.so.1",
                "/usr/lib/libWPEBackend-fdo-1.0.so.1",
                "libWPEBackend-fdo-1.0.so.1"
            };
            
            bool loaded = false;
            for (const char* path : paths) {
                if (wpe_loader_init(path)) {
                    LOG_INFO << "WPE Backend loaded successfully from: " << path;
                    loaded = true;
                    break;
                }
            }
            
            if (!loaded) {
                LOG_ERROR << "CRITICAL: Could not find libWPEBackend-fdo-1.0.so.1 in common paths!";
            }

            // --- MULTI-CORE OPTIMIZATIONS ---
            // Set Skia painting threads to use all available cores
            int cores = std::thread::hardware_concurrency();
            if (cores > 0) {
                std::string cores_str = std::to_string(cores);
                g_setenv("WEBKIT_SKIA_PAINTING_THREADS", cores_str.c_str(), TRUE);
                LOG_INFO << "Setting WEBKIT_SKIA_PAINTING_THREADS to: " << cores_str;
            }
            // Ensure hardware acceleration is favored
            g_setenv("WEBKIT_SKIA_ENABLE_CPU_RENDERING", "0", TRUE);
            g_setenv("WEBKIT_FORCE_COMPOSITING_MODE", "1", TRUE);

            wpe_fdo_initialize_shm();

            LOG_INFO << "Shared WPE WebKit worker thread started";
            
            g_main_loop_run(s_worker_loop);

            g_main_context_pop_thread_default(s_worker_context);
            // We'll let the static objects be cleaned up at process exit or manually later 
            // to avoid race conditions with joins.
        }

        void WebWidget::on_title_notify(WebKitWebView* web_view, GParamSpec*, WebWidget* self) {
            if (!self || !self->m_web_view) return;
            const char* title_str = webkit_web_view_get_title(web_view);
            if (!title_str) return;
            std::string title = title_str;

            // BEACON HANDLING: Intercept scroll updates masked as title changes
            if (title.find("HORIZON_SCROLL:") == 0) {
                double sy, sx, ch, cw;
                if (sscanf(title.c_str() + 15, "%lf %lf %lf %lf", &sy, &sx, &ch, &cw) == 4) {
                    bool changed = false;
                    {
                        std::lock_guard<std::mutex> lock{self->m_scroll_mutex};
                        if (std::abs(self->m_target_scroll_y - sy) >= 1.0 || 
                            std::abs(self->m_target_content_h - ch) >= 1.0) {
                            self->m_target_scroll_y = sy;
                            self->m_target_scroll_x = sx;
                            self->m_target_content_h = ch;
                            self->m_target_content_w = cw;
                            changed = true;
                        }
                    }
                    if (changed) {
                        self->m_scroll_dirty = true;
                        self->invalidate();
                    }
                    return; // Eat the beacon update, don't propagate to UI
                }
            }

            {
                std::lock_guard<std::mutex> lock(self->m_metadata_mutex);
                self->m_cached_title = title;
            }

            if (self->application()) {
                self->application()->post_task([self, title]() {
                    std::string t = title;
                    self->when_title_changed.run(t);
                });
            }
        }

        void WebWidget::on_uri_notify(WebKitWebView* web_view, GParamSpec*, WebWidget* self) {
            if (!self || !self->m_web_view) return;
            const char* uri_str = webkit_web_view_get_uri(web_view);
            std::string url = uri_str ? uri_str : "";

            {
                std::lock_guard<std::mutex> lock(self->m_metadata_mutex);
                self->m_cached_url = url;
            }

            if (self->application()) {
                self->application()->post_task([self, url]() {
                    std::string u = url;
                    self->when_url_changed.run(u);
                });
            }
        }

        void WebWidget::on_load_changed(WebKitWebView*, int load_event, WebWidget* self) {
            if (!self || !self->m_web_view) return;
            LOG_INFO << "Load changed for WebWidget: " << self << " Event: " << load_event;
            bool loading = (load_event != 3); // 3 == WEBKIT_LOAD_FINISHED
            if (self->application()) {
                self->application()->post_task([self, loading]() {
                    bool l = loading;
                    self->when_loading_changed.run(l);
                });
            }
            if (load_event == 3 && self->m_web_view) { // Finished
                 // Redundant injection to catch late-initializing native scrollbars
                 webkit_web_view_evaluate_javascript(self->m_web_view, 
                    "const style = document.createElement('style');"
                    "style.textContent = '::-webkit-scrollbar { display: none !important; }';"
                    "document.head.appendChild(style);", -1, NULL, NULL, NULL, NULL, NULL);
                 webkit_web_view_evaluate_javascript(self->m_web_view, 
                    "if(window._horizon_send_scroll) _horizon_send_scroll();", -1, NULL, NULL, NULL, NULL, NULL);
            }
        }

        void WebWidget::on_progress_notify(WebKitWebView* web_view, GParamSpec*, WebWidget* self) {
            if (!self || !self->m_web_view) return;
            double progress = webkit_web_view_get_estimated_load_progress(web_view);
            if (self->application()) {
                self->application()->post_task([self, progress]() {
                    double p = progress;
                    self->when_progress_changed.run(p);
                });
            }
            if (progress >= 1.0 && self->m_web_view) {
                 webkit_web_view_evaluate_javascript(self->m_web_view, 
                    "if(window._horizon_send_scroll) _horizon_send_scroll();", -1, NULL, NULL, NULL, NULL, NULL);
            }
        }

        void WebWidget::on_mouse_target_changed(WebKitWebView*, void*, uint32_t, WebWidget* self) {}

        std::string WebWidget::get_title() const {
             std::lock_guard<std::mutex> lock(m_metadata_mutex);
             return m_cached_title;
        }

        std::string WebWidget::get_url() const {
             std::lock_guard<std::mutex> lock(m_metadata_mutex);
             return m_cached_url;
        }

        void WebWidget::on_frame_exported(void *data, struct wpe_fdo_shm_exported_buffer *buffer)
        {
            auto *self = static_cast<WebWidget *>(data);
            if (!self || !self->m_exportable) return;
            struct wl_shm_buffer *shm_buffer = wpe_fdo_shm_exported_buffer_get_shm_buffer(buffer);
            if (!shm_buffer)
            {
                wpe_view_backend_exportable_fdo_dispatch_release_shm_exported_buffer(self->m_exportable, buffer);
                return;
            }

            int width = wl_shm_buffer_get_width(shm_buffer);
            int height = wl_shm_buffer_get_height(shm_buffer);
            void *buffer_data = wl_shm_buffer_get_data(shm_buffer);
            int stride = wl_shm_buffer_get_stride(shm_buffer);

            {
                std::lock_guard<std::mutex> lock(self->m_surface_mutex);
                if (!self->m_cairo_surface ||
                    cairo_image_surface_get_width(self->m_cairo_surface) != width ||
                    cairo_image_surface_get_height(self->m_cairo_surface) != height)
                {
                    if (self->m_cairo_surface) cairo_surface_destroy(self->m_cairo_surface);
                    self->m_cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
                }

                unsigned char *dest = cairo_image_surface_get_data(self->m_cairo_surface);
                if (dest && buffer_data) {
                    cairo_surface_flush(self->m_cairo_surface);
                    std::memcpy(dest, buffer_data, stride * height);
                    cairo_surface_mark_dirty(self->m_cairo_surface);
                }
            }

            wpe_view_backend_exportable_fdo_dispatch_release_shm_exported_buffer(self->m_exportable, buffer);
            wpe_view_backend_exportable_fdo_dispatch_frame_complete(self->m_exportable);
            
            static int frame_count = 0;
            if (++frame_count % 60 == 0) LOG_INFO << "Frame exported for WebWidget: " << self;

            if (self->application()) {
                self->application()->post_task([self]() {
                    // Frame-Synced Invalidation:
                    // We only invalidate if WebKit sent a new frame OR the scrollbar state is dirty.
                    // This couples the OS scrollbars to the browser engine's vsync.
                    bool scroll_expected = self->m_scroll_dirty.exchange(false);
                    self->invalidate();
                });
            }
        }

        void WebWidget::update_scrollbars() {
            std::lock_guard<std::mutex> lock(m_scroll_mutex);
            
            // Sync logical state with quantized target state
            m_scroll_y = m_target_scroll_y;
            m_scroll_x = m_target_scroll_x;
            
            // FALLBACK: If content height is unknown, assume it matches the view height
            m_content_height = (m_target_content_h > 0) ? m_target_content_h : height();
            m_content_width = (m_target_content_w > 0) ? m_target_content_w : width();

            if (height() <= 0 || width() <= 0) return;

            // PERSISTENT VISIBILITY: Always show the gutter for responsiveness
            m_show_v_scroll = true; 
            m_show_h_scroll = m_content_width > width() + 1;

            if (m_show_v_scroll) {
                m_v_track_x = x() + width() - SCROLLBAR_SIZE - 2;
                m_v_track_y = y() + 2;
                m_v_track_w = SCROLLBAR_SIZE;
                m_v_track_h = height() - 4 - (m_show_h_scroll ? SCROLLBAR_SIZE : 0);

                double visible_ratio = (double)height() / m_content_height;
                if (!std::isfinite(visible_ratio)) visible_ratio = 1.0;
                m_v_thumb_h = std::max(20, (int)(m_v_track_h * visible_ratio));
                
                double scrollable_height = m_content_height - height();
                double scroll_ratio = (scrollable_height > 0) ? (m_scroll_y / scrollable_height) : 0;
                scroll_ratio = std::max(0.0, std::min(1.0, scroll_ratio));
                
                m_v_thumb_y = m_v_track_y + (int)(scroll_ratio * (m_v_track_h - m_v_thumb_h));
            }

            if (m_show_h_scroll) {
                m_h_track_x = x() + 2;
                m_h_track_y = y() + height() - SCROLLBAR_SIZE - 2;
                m_h_track_w = width() - 4 - (m_show_v_scroll ? SCROLLBAR_SIZE : 0);
                m_h_track_h = SCROLLBAR_SIZE;

                double visible_ratio = (double)width() / m_content_width;
                if (!std::isfinite(visible_ratio)) visible_ratio = 1.0;
                m_h_thumb_w = std::max(20, (int)(m_h_track_w * visible_ratio));
                
                double scrollable_width = m_content_width - width();
                double scroll_ratio = (scrollable_width > 0) ? (m_scroll_x / scrollable_width) : 0;
                scroll_ratio = std::max(0.0, std::min(1.0, scroll_ratio));
                
                m_h_thumb_x = m_h_track_x + (int)(scroll_ratio * (m_h_track_w - m_h_thumb_w));
            }
        }
    
        void WebWidget::handle_ui_scroll(int x, int y) {
            if (s_worker_context) {
                std::string js;
                if (x >= 0 && y >= 0) js = "window.scrollTo(" + std::to_string(x) + "," + std::to_string(y) + ")";
                else if (x >= 0) js = "window.scrollTo(" + std::to_string(x) + ", window.scrollY)";
                else if (y >= 0) js = "window.scrollTo(window.scrollX," + std::to_string(y) + ")";
                
                if (js.empty()) return;

                struct JSData { WebKitWebView* v; std::string s; };
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* d = (JSData*)data;
                    if (d->v) webkit_web_view_evaluate_javascript(d->v, d->s.c_str(), -1, NULL, NULL, NULL, NULL, NULL);
                    delete d;
                    return FALSE;
                }, new JSData{m_web_view, js});
            }
        }

        void WebWidget::load_url(const std::string &url)
        {
            if (!m_initialized) init_wpe();
            
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* pair = static_cast<std::pair<WebWidget*, std::string>*>(data);
                    if (pair->first->m_web_view) {
                        LOG_INFO << "Loading URI: " << pair->second << " in WebWidget: " << pair->first;
                        webkit_web_view_load_uri(pair->first->m_web_view, pair->second.c_str());
                    } else {
                        LOG_ERROR << "Failed to load URI: m_web_view is NULL for WebWidget: " << pair->first;
                    }
                    delete pair;
                    return FALSE;
                }, new std::pair<WebWidget*, std::string>(this, url));
            }
        }

        void WebWidget::draw(GraphicsContext &ctx)
        {
            update_scrollbars();
            
            ctx.setColor(background_color());
            ctx.fillRect(x(), y(), width(), height());

            std::lock_guard<std::mutex> lock(m_surface_mutex);
            if (m_cairo_surface)
            {
                cairo_t *cr = (cairo_t *)ctx.getNativeContext();
                cairo_set_source_surface(cr, m_cairo_surface, x(), y());
                cairo_paint(cr);
            }

            // Draw Horizon-style scrollbars (Aqua feel) - IMPROVED GLASS TUBE
            if (m_show_v_scroll) {
                Color electric_blue = Color("#3a86ff");
                Color deep_blue = Color("#1a44c5");
                Color shine_color = Color("#ffffff").with_alpha(0.85f);
                Color track_color = Color("#f8f8f8");
                Color border_color = Color("#000000").with_alpha(0.3f);
                int size = 15; // Increased width for better interaction
                CornerRadius r(size / 2);

                // A. Track
                ctx.setColor(track_color);
                ctx.fillRect(m_v_track_x, m_v_track_y, m_v_track_w, m_v_track_h, r);
                ctx.setColor(Color("#e0e0e0"));
                ctx.drawRect(m_v_track_x, m_v_track_y, m_v_track_w, m_v_track_h, r, 0.5f);
                
                // B. Thumb - Multi-Layer 3D Glass Pipeline
                if (m_content_height > height() + 1 || m_target_content_h <= 0) {
                    // Layer 1: Cylindrical Volume
                    ctx.fillLinearGradientRect(m_v_track_x, m_v_thumb_y, m_v_track_w, m_v_thumb_h, 
                                              deep_blue, electric_blue, false, r);

                    // Layer 2: End Caps
                    int cap_size = std::min(6, m_v_thumb_h / 3);
                    ctx.fillLinearGradientRect(m_v_track_x, m_v_thumb_y, m_v_track_w, cap_size, 
                                              deep_blue.with_alpha(0.6f), Color(0.0f, 0.0f, 0.0f, 0.0f), true, CornerRadius(r.top_left, r.top_right, 0, 0));
                    ctx.fillLinearGradientRect(m_v_track_x, m_v_thumb_y + m_v_thumb_h - cap_size, m_v_track_w, cap_size, 
                                              Color(0.0f, 0.0f, 0.0f, 0.0f), deep_blue.with_alpha(0.6f), true, CornerRadius(0, 0, r.bottom_right, r.bottom_left));

                    // Layer 3: THE SHINE (Vertical Glass Reflection)
                    ctx.fillLinearGradientRect(m_v_track_x + 3, m_v_thumb_y + 4, 4, m_v_thumb_h - 8, 
                                              shine_color, shine_color.with_alpha(0.1f), false, CornerRadius(2));

                    // Layer 4: Sharpening Border
                    ctx.setColor(border_color);
                    ctx.drawRect(m_v_track_x, m_v_thumb_y, m_v_track_w, m_v_thumb_h, r, 0.8f);
                }
            }

            if (m_show_h_scroll) {
                Color electric_blue = Color("#3a86ff");
                Color deep_blue = Color("#1a44c5");
                Color shine_color = Color("#ffffff").with_alpha(0.8f);
                Color track_color = Color("#f5f5f5");
                Color border_color = Color("#000000").with_alpha(0.4f);
                CornerRadius r(m_h_track_h / 2);

                // A. Track
                ctx.setColor(track_color);
                ctx.fillRect(m_h_track_x, m_h_track_y, m_h_track_w, m_h_track_h, r);
                ctx.setColor(Color("#dddddd"));
                ctx.drawRect(m_h_track_x, m_h_track_y, m_h_track_w, m_h_track_h, r, 0.5f);

                // B. Thumb
                ctx.fillLinearGradientRect(m_h_thumb_x, m_h_track_y, m_h_thumb_w, m_h_track_h, 
                                          deep_blue, electric_blue, true, r); // Opposite for horizontal

                int cap_size = std::min(5, m_h_thumb_w / 4);
                ctx.fillLinearGradientRect(m_h_thumb_x, m_h_track_y, cap_size, m_h_track_h, 
                                          deep_blue.with_alpha(0.5f), Color(0.0f, 0.0f, 0.0f, 0.0f), false, CornerRadius(r.top_left, 0, 0, r.bottom_left));
                ctx.fillLinearGradientRect(m_h_thumb_x + m_h_thumb_w - cap_size, m_h_track_y, cap_size, m_h_track_h, 
                                          Color(0.0f, 0.0f, 0.0f, 0.0f), deep_blue.with_alpha(0.5f), false, CornerRadius(0, r.top_right, r.bottom_right, 0));

                ctx.fillLinearGradientRect(m_h_thumb_x + 3, m_h_track_y + 2, m_h_thumb_w - 6, 3, 
                                          shine_color, shine_color.with_alpha(0.2f), true, CornerRadius(2));

                ctx.setColor(border_color);
                ctx.drawRect(m_h_thumb_x, m_h_track_y, m_h_thumb_w, m_h_track_h, r, 1.0f);
            }
        }

        void WebWidget::calculate_layout()
        {
            Widget::calculate_layout();
            if (m_initialized && m_backend && width() > 0 && height() > 0 && s_worker_context)
            {
                if (width() == m_last_dispatched_width && height() == m_last_dispatched_height) return;
                
                m_last_dispatched_width = width();
                m_last_dispatched_height = height();

                struct ResizeData {
                    struct wpe_view_backend* backend;
                    int w;
                    int h;
                };
                auto* rd = new ResizeData{m_backend, width(), height()};
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* d = static_cast<ResizeData*>(data);
                    wpe_view_backend_dispatch_set_size(d->backend, d->w, d->h);
                    delete d;
                    return FALSE;
                }, rd);
            }
            else if (!m_initialized && width() > 0 && height() > 0)
            {
                init_wpe();
            }
        }

        void WebWidget::reload() { 
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebWidget*>(data);
                    if (self->m_web_view) webkit_web_view_reload(self->m_web_view);
                    return FALSE;
                }, this);
            }
        }
        void WebWidget::stop_loading() { 
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebWidget*>(data);
                    if (self->m_web_view) webkit_web_view_stop_loading(self->m_web_view);
                    return FALSE;
                }, this);
            }
        }
        void WebWidget::go_back() { 
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebWidget*>(data);
                    if (self->m_web_view) webkit_web_view_go_back(self->m_web_view);
                    return FALSE;
                }, this);
            }
        }
        void WebWidget::go_forward() { 
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebWidget*>(data);
                    if (self->m_web_view) webkit_web_view_go_forward(self->m_web_view);
                    return FALSE;
                }, this);
            }
        }

        bool WebWidget::can_go_back() const { return m_web_view && webkit_web_view_can_go_back(m_web_view); }
        bool WebWidget::can_go_forward() const { return m_web_view && webkit_web_view_can_go_forward(m_web_view); }

    } // namespace web
} // namespace horizon
