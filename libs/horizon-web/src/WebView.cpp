#include "horizon/web/WebView.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Application.hpp"
#include "horizon/WaylandWindow.hpp"
#include <cstring>
#include <wayland-server-core.h>
#include <jsc/jsc.h>
#include <wpe/webkit.h>
#include <wpe/fdo.h>
#include <wpe/unstable/fdo-shm.h>
#include <glib.h>
#include <thread>
#include <mutex>
#include <cmath>


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
            case 273: return 3; // BTN_RIGHT
            case 274: return 2; // BTN_MIDDLE
            default: return button;
            }
        }

        std::thread WebView::s_worker_thread;
        GMainContext* WebView::s_worker_context = nullptr;
        GMainLoop* WebView::s_worker_loop = nullptr;
        bool WebView::s_running = false;
        std::mutex WebView::s_worker_mutex;

        static int s_instance_count = 0;
        static std::mutex s_instance_mutex;

        WebView::WebView()
        {
            set_focusable(true);
            set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
            // Thumbs are drawn directly now

            // Initialization is now managed by a shared static method called from the worker thread

            // Mouse Press
            when_mouse_press.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    if (ctx.button == 274) return; // DISALLOW MIDDLE BUTTON (Prevent Autoscroll Modal)
                    set_focus(true);
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
                                                             (uint32_t)(g_get_monotonic_time() / 1000), 
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             map_to_wpe_button(ctx.button),
                                                             1,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers)};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebView*, wpe_input_pointer_event*>*>(data);
                        if (d->first->m_backend) {
                            // Heartbeat focus on click to refresh user gesture timer
                            wpe_view_backend_add_activity_state(d->first->m_backend, wpe_view_activity_state_visible | wpe_view_activity_state_focused | wpe_view_activity_state_in_window);
                            wpe_view_backend_dispatch_pointer_event(d->first->m_backend, d->second);
                        }
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebView*, wpe_input_pointer_event*>(this, event));
                });

            // Mouse Release
            when_mouse_release.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    if (ctx.button == 274) return; // DISALLOW MIDDLE BUTTON
                    m_is_dragging_v = false;
                    m_is_dragging_h = false;

                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_button,
                                                             (uint32_t)(g_get_monotonic_time() / 1000),
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             map_to_wpe_button(ctx.button),
                                                             0,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers)};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebView*, wpe_input_pointer_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_pointer_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebView*, wpe_input_pointer_event*>(this, event));
                });

            // Mouse Move
            when_mouse_move.connect(
                [this](MouseMoveEventContext &ctx)
                {
                    // TRANSITION SHIELD: Ignore mouse moves for 1s after entering FS to prevent UI popups/aborts
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_fullscreen_time).count();
                    if (m_is_fullscreen && elapsed_ms < 1000) {
                        return;
                    }

                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_motion,
                                                             (uint32_t)(g_get_monotonic_time() / 1000),
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             0,
                                                             0,
                                                             0};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebView*, wpe_input_pointer_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_pointer_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebView*, wpe_input_pointer_event*>(this, event));
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
                            (uint32_t)(g_get_monotonic_time() / 1000),
                            (int)(ctx.x - x()),
                            (int)(ctx.y - y()),
                            axis,
                            value,
                            wpe_mods};

                        g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                            auto* d = static_cast<std::pair<WebView*, wpe_input_axis_event*>*>(data);
                            if (d->first->m_backend) wpe_view_backend_dispatch_axis_event(d->first->m_backend, d->second);
                            delete d->second;
                            delete d;
                            return FALSE;
                        }, new std::pair<WebView*, wpe_input_axis_event*>(this, event));
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
                    // ESCAPE BYPASS: Unconditional Nova-level handle during Immersive Mode
                    if (m_is_fullscreen && ctx.keysym == 0xFF1B) {
                        LOG_INFO << "[WEB] ESC pressed. Nuclear Reset of Fullscreen State.";
                        on_leave_fullscreen(NULL, this);
                        return;
                    }

                    auto* event = new wpe_input_keyboard_event{(uint32_t)(g_get_monotonic_time() / 1000), ctx.keysym, ctx.key + 8, true,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers)};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebView*, wpe_input_keyboard_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_keyboard_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebView*, wpe_input_keyboard_event*>(this, event));
                });

            when_key_release.connect(
                [this](KeyEventContext &ctx)
                {
                    auto* event = new wpe_input_keyboard_event{(uint32_t)(g_get_monotonic_time() / 1000), ctx.keysym, ctx.key + 8, false,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers)};

                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebView*, wpe_input_keyboard_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_keyboard_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebView*, wpe_input_keyboard_event*>(this, event));
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

        WebView::~WebView()
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
            // Joining is now handled explicitly in WebView::shutdown().
        }

        void WebView::init_wpe()
        {
            if (m_initialized)
                return;

            ensure_worker_thread();
            
            {
                std::lock_guard<std::mutex> ilock(s_worker_mutex);
                s_instance_count++;
            }

            // Create backend/webview in the worker thread
            // USE HIGH PRIORITY: Must match load_url priority to avoid race conditions
            g_main_context_invoke_full(s_worker_context, G_PRIORITY_HIGH, (GSourceFunc)+[](void* data) -> gboolean {
                auto* self = static_cast<WebView*>(data);
                
                struct FullscreenData {
                    WebView* self;
                    bool fs;
                };

                static auto fs_callback = [](void* data, bool fullscreen) {
                    auto* self = static_cast<WebView*>(data);
                    LOG_INFO << "[WEB] Backend requested fullscreen via reserved0: " << (fullscreen ? "ON" : "OFF");
                    if (self->application()) {
                        self->application()->post_task([self, fullscreen]() {
                            bool fs = fullscreen;
                            self->when_fullscreen_changed.run(fs);
                        });
                    }
                };

                static struct wpe_view_backend_exportable_fdo_client client = {
                    .export_buffer_resource = nullptr,
                    .export_dmabuf_resource = [](void *data, struct wpe_view_backend_exportable_fdo_dmabuf_resource *dmabuf)
                    {
                        auto *self = static_cast<WebView *>(data);
                        // LOG_INFO << "[WEB-DMA] DMABUF exported: " << dmabuf->width << "x" << dmabuf->height;
                        
                        // ACK the frame immediately to keep the engine pumping
                        wpe_view_backend_exportable_fdo_dispatch_frame_complete(self->m_exportable);
                        
                        // Release the resource immediately as we aren't yet importing it to GL
                        wpe_view_backend_exportable_fdo_dispatch_release_buffer(self->m_exportable, dmabuf->buffer_resource);
                    },
                    .export_shm_buffer =
                        [](void *data, struct wpe_fdo_shm_exported_buffer *buffer)
                    {
                        auto *self = static_cast<WebView *>(data);
                        WebView::on_frame_exported(self, buffer);
                    },
                    ._wpe_reserved0 = reinterpret_cast<void(*)(void)>(static_cast<void(*)(void*, bool)>(fs_callback)),
                    ._wpe_reserved1 = nullptr};

                int initial_width = self->width() > 0 ? self->width() : 800;
                int initial_height = self->height() > 0 ? self->height() : 600;

                self->m_exportable = wpe_view_backend_exportable_fdo_create(&client, self, initial_width, initial_height);
                self->m_backend = wpe_view_backend_exportable_fdo_get_view_backend(self->m_exportable);
                
                // ENSURE ACTIVITY STATE: Tell the engine we are visible/focused/in-window immediately
                wpe_view_backend_add_activity_state(self->m_backend, 7); // 7 = VISIBLE | FOCUSED | IN_WINDOW

                // ENSURE BASE STATE: Explicitly tell the engine we are NOT in fullscreen at startup
                wpe_view_backend_platform_set_fullscreen(self->m_backend, FALSE);

                auto *webkit_backend = webkit_web_view_backend_new(self->m_backend, nullptr, nullptr);

                // --- PERFORMANCE OPTIMIZATIONS ---
                WebKitSettings* settings = webkit_settings_new();
                webkit_settings_set_enable_fullscreen(settings, TRUE);
                webkit_settings_set_javascript_can_open_windows_automatically(settings, TRUE);
                
                // Verify it actually stuck
                gboolean fs_enabled = FALSE;
                g_object_get(settings, "enable-fullscreen", &fs_enabled, NULL);
                LOG_INFO << "[WEB] WebKitSettings enable-fullscreen verified: " << (fs_enabled ? "TRUE" : "FALSE");

                webkit_settings_set_enable_developer_extras(settings, TRUE);
                webkit_settings_set_enable_media_stream(settings, TRUE);
                webkit_settings_set_enable_webgl(settings, TRUE);
                webkit_settings_set_user_agent(settings, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
                webkit_settings_set_allow_modal_dialogs(settings, TRUE);
                webkit_settings_set_javascript_can_access_clipboard(settings, TRUE);
                webkit_settings_set_media_playback_requires_user_gesture(settings, FALSE);
                webkit_settings_set_media_playback_allows_inline(settings, TRUE);
                
                // version-safe check for potential properties
                if (g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "enable-back-forward-navigation-gestures")) {
                    g_object_set(settings, "enable-back-forward-navigation-gestures", TRUE, NULL);
                }
                
                self->m_web_view = webkit_web_view_new(webkit_backend);
                webkit_web_view_set_settings(self->m_web_view, settings);
                g_object_unref(settings);

                LOG_INFO << "WebView created with performance optimizations for WebView: " << self;

                g_signal_connect(self->m_web_view, "notify::title", G_CALLBACK(on_title_notify), self);
                g_signal_connect(self->m_web_view, "notify::uri", G_CALLBACK(on_uri_notify), self);
                g_signal_connect(self->m_web_view, "notify::estimated-load-progress", G_CALLBACK(on_progress_notify), self);
                g_signal_connect(self->m_web_view, "load-changed", G_CALLBACK(on_load_changed), self);
                g_signal_connect(self->m_web_view, "resource-load-started", G_CALLBACK(+[](WebKitWebView*, WebKitWebResource* resource, WebKitURIRequest* request, void*) {
                    const char* uri = webkit_uri_request_get_uri(request);
                    LOG_INFO << "[WEB-RESOURCE] Resource load started: " << (uri ? uri : "unknown");
                }), NULL);
                g_signal_connect(self->m_web_view, "load-failed", G_CALLBACK(+[](WebKitWebView*, WebKitLoadEvent, const char* url, GError* error, void*) -> gboolean {
                    LOG_ERROR << "[WEB-CRITICAL] Load failed for " << url << ": " << (error ? error->message : "Unknown error");
                    return FALSE;
                }), NULL);
                g_signal_connect(self->m_web_view, "mouse-target-changed", G_CALLBACK(on_mouse_target_changed), self);
                unsigned long id_fs = g_signal_connect(self->m_web_view, "enter-fullscreen", G_CALLBACK(WebView::on_enter_fullscreen), self);
                unsigned long id_ls = g_signal_connect(self->m_web_view, "leave-fullscreen", G_CALLBACK(WebView::on_leave_fullscreen), self);
                unsigned long id_pr = g_signal_connect(self->m_web_view, "permission-request", G_CALLBACK(WebView::on_permission_request), self);
                unsigned long id_dp = g_signal_connect(self->m_web_view, "decide-policy", G_CALLBACK(WebView::on_decide_policy), self);
                g_signal_connect(self->m_web_view, "context-menu", G_CALLBACK(WebView::on_context_menu), self);
                
                g_signal_connect(self->m_web_view, "web-process-terminated", G_CALLBACK(+[](WebKitWebView*, WebKitWebProcessTerminationReason reason, void*) {
                    LOG_ERROR << "[WEB-CRITICAL] Web process terminated! Reason: " << reason;
                }), NULL);

                LOG_INFO << "[WEB] Signal registration IDs: enter_fs=" << id_fs << ", leave_fs=" << id_ls << ", perm_req=" << id_pr << ", decide_policy=" << id_dp;

                // --- SIGNAL TRACING ---
                guint n_ids;
                guint* ids = g_signal_list_ids(G_TYPE_FROM_INSTANCE(self->m_web_view), &n_ids);
                for(guint i=0; i<n_ids; i++) {
                    const char* name = g_signal_name(ids[i]);
                    if (strstr(name, "fullscreen") || strstr(name, "full-screen") || strstr(name, "policy")) {
                        LOG_INFO << "[WEB-DEBUG] Interesting Signal found: " << name;
                    }
                }
                g_free(ids);
                WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(self->m_web_view);
                
                const char* script_source = 
                    "const inject = () => {"
                    "  if (window._horizon_injected) return;"
                    "  const bridge = (msg) => { document.title = msg; };"
                    "  "
                    "  const origRF = Element.prototype.requestFullscreen || Element.prototype.webkitRequestFullscreen;"
                    "  if (origRF) {"
                    "    const wrap = function() {"
                    "      bridge('FS_LOG:requestFullscreen');"
                    "      const p = origRF.apply(this, arguments);"
                    "      if (p && p.catch) {"
                    "        p.catch(err => console.error('[FS] ' + err.message));"
                    "      }"
                    "      return p;"
                    "    };"
                    "    Element.prototype.requestFullscreen = wrap;"
                    "    Element.prototype.webkitRequestFullscreen = wrap;"
                    "  }"
                    "  window._horizon_injected = true;"
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


                // REGISTER CHANNEL BRIDGE: Dedicated signals for zero-copy communication
                webkit_user_content_manager_register_script_message_handler(manager, "nova_enter", NULL);
                webkit_user_content_manager_register_script_message_handler(manager, "nova_exit", NULL);
                
                g_signal_connect(manager, "script-message-received::nova_enter", G_CALLBACK(+[](WebKitUserContentManager*, WebKitJavascriptResult*, WebView* self) {
                    LOG_INFO << "[WEB-BRIDGE] Direct trigger: ENTER FS";
                    self->on_enter_fullscreen(NULL, self);
                }), self);

                g_signal_connect(manager, "script-message-received::nova_exit", G_CALLBACK(+[](WebKitUserContentManager*, WebKitJavascriptResult*, WebView* self) {
                    LOG_INFO << "[WEB-BRIDGE] Direct trigger: EXIT FS";
                    self->on_leave_fullscreen(NULL, self);
                }), self);

                // PERSISTENT NUCLEAR SHIM: V9.2 (Channel Bridge)
                const char* nuclear_source = 
                    "window._fsStartTime = Date.now();"
                    "const msgEnter = () => { if(window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.nova_enter) window.webkit.messageHandlers.nova_enter.postMessage(null); };"
                    "const msgExit = () => { if(window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.nova_exit) window.webkit.messageHandlers.nova_exit.postMessage(null); };"
                    "const wrapEnter = function() { "
                    "  msgEnter(); "
                    "  this.classList.add('horizon-fs'); "
                    "  document.documentElement.classList.add('horizon-fs'); "
                    "  return Promise.resolve(); "
                    "};"
                    "const wrapExit = function() { "
                    "  if (Date.now() - window._fsStartTime > 500) { "
                    "    msgExit(); "
                    "  } "
                    "  return Promise.resolve(); "
                    "};"
                    "try { "
                    "  Element.prototype.requestFullscreen = Element.prototype.webkitRequestFullscreen = wrapEnter; "
                    "  document.exitFullscreen = document.webkitExitFullscreen = wrapExit; "
                    "} catch(e) {}"
                    "window.addEventListener('click', (e) => {"
                    "  const btn = e.target.closest('.ytp-fullscreen-button');"
                    "  if (btn) { msgExit(); }"
                    "}, true);"
                    "window.addEventListener('keydown', (e) => {"
                    "  if (e.keyCode === 27 || e.key === 'f' || e.key === 'F') { msgEnter(); } "
                    "}, true);"
                    "Object.defineProperty(screen, 'width', { value: 1920, configurable: true });"
                    "Object.defineProperty(screen, 'height', { value: 1080, configurable: true });"
                    "Object.defineProperty(window, 'innerWidth', { value: 1920, configurable: true });"
                    "Object.defineProperty(window, 'innerHeight', { value: 1080, configurable: true });"
                    "try {"
                    "  const getFsEl = () => document.documentElement.classList.contains('horizon-fs') ? (document.querySelector('#movie_player') || document.querySelector('video')) : null;"
                    "  Object.defineProperty(document, 'fullscreenElement', { get: getFsEl, configurable: true });"
                    "  Object.defineProperty(document, 'webkitFullscreenElement', { get: getFsEl, configurable: true });"
                    "  Object.defineProperty(document, 'webkitIsFullScreen', { get: () => document.documentElement.classList.contains('horizon-fs'), configurable: true });"
                    "} catch(e) {}"
                    "const wakeUp = setInterval(() => {"
                    "  if (!document.documentElement.classList.contains('horizon-fs')) { clearInterval(wakeUp); return; }"
                    "  window.dispatchEvent(new Event('resize'));"
                    "  document.dispatchEvent(new Event('fullscreenchange'));"
                    "  const v = document.querySelector('video'); if(v && v.paused) v.play();"
                    "  if(Date.now() - window._fsStartTime > 2000) clearInterval(wakeUp);"
                    "}, 250);";

                WebKitUserScript* n_script = webkit_user_script_new(
                    nuclear_source,
                    WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                    WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
                    NULL, NULL);
                webkit_user_content_manager_add_script(manager, n_script);
                webkit_user_script_unref(n_script);

                WebKitUserStyleSheet* fs_style = webkit_user_style_sheet_new(
                    "html.horizon-fs #secondary, html.horizon-fs #comments, html.horizon-fs ytd-masthead, html.horizon-fs #masthead-container, html.horizon-fs #below { display: none !important; }"
                    "html.horizon-fs #player, html.horizon-fs .html5-video-player, html.horizon-fs #movie_player { background: black !important; width: 100vw !important; height: 100vh !important; position: fixed !important; top: 0 !important; left: 0 !important; z-index: 2147483647 !important; visibility: visible !important; }"
                    "html.horizon-fs video { background: transparent !important; width: 100% !important; height: 100% !important; position: relative !important; z-index: 2147483647 !important; visibility: visible !important; }",
                    WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                    WEBKIT_USER_STYLE_LEVEL_USER,
                    NULL, NULL);
                webkit_user_content_manager_add_style_sheet(manager, fs_style);
                webkit_user_style_sheet_unref(fs_style);

                return FALSE;
            }, this, NULL);

            m_initialized = true;
        }

        void WebView::shutdown() {
            std::lock_guard<std::mutex> lock(s_worker_mutex);
            if (!s_running) return;

            LOG_INFO << "Global WebView shutdown initiated...";
            
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
            LOG_INFO << "Global WebView shutdown complete.";
        }

        void WebView::ensure_worker_thread()
        {
            std::lock_guard<std::mutex> lock(s_worker_mutex);
            if (s_running) return;

            s_running = true;
            s_worker_thread = std::thread(&WebView::worker_thread_func);
            
            // Wait for context to be initialized
            while (!s_worker_context) {
                std::this_thread::yield();
            }
        }

        void WebView::worker_thread_func() 
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

            // --- WAYFIRE "ZERO ISOLATION" MODE ---
            // Total sandbox disable to ensure sub-processes see the Wayland socket.
            g_setenv("WEBKIT_FORCE_SANDBOX", "0", TRUE);
            g_setenv("WEBKIT_DISABLE_SANDBOX_THIS_IS_DANGEROUS", "1", TRUE);
            
            // PURE SOFTWARE PATH (No GPU Process, No Compositing)
            g_setenv("WEBKIT_DISABLE_GPU_PROCESS", "1", TRUE);
            g_setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", TRUE);
            g_setenv("WEBKIT_SKIA_ENABLE_CPU_RENDERING", "1", TRUE);
            g_setenv("WEBKIT_DISABLE_ACCELERATED_2D_CANVAS", "1", TRUE);
            
            // DEBUGGING & STABILITY
            g_setenv("WPE_DEBUG", "1", TRUE);
            g_setenv("WPE_FDO_FORCE_SHM", "1", TRUE);
            
            // DEBUGGING & STABILITY
            g_setenv("WPE_DEBUG", "1", TRUE);
            g_setenv("WPE_FDO_FORCE_SHM", "1", TRUE);
            g_setenv("WEBKIT_DISABLE_WAYLAND_DISPLAY_CHECK", "1", TRUE);

            LOG_INFO << "[WEB] Initializing WPE for SHM";
            wpe_fdo_initialize_shm();

            LOG_INFO << "Shared WPE WebKit worker thread started";
            
            g_main_loop_run(s_worker_loop);

            g_main_context_pop_thread_default(s_worker_context);
            // We'll let the static objects be cleaned up at process exit or manually later 
            // to avoid race conditions with joins.
        }


        void WebView::on_title_notify(void*, void*, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return;
            const char* title_str = webkit_web_view_get_title(self->m_web_view);
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
                    }
                    return;
                }
            }

            if (title.find("FS_ERROR:") == 0 || title.find("FS_LOG:") == 0) {
                LOG_INFO << "[WEB-JS] " << title;
                if (title.find("FS_LOG:requestFullscreen") == 0) {
                    LOG_INFO << "[WEB] Adaptive Trigger: JS requested fullscreen.";
                    on_enter_fullscreen(nullptr, self);
                }
                return; // Don't set as actual title
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

        void WebView::on_uri_notify(void*, void*, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return;
            const char* uri_str = webkit_web_view_get_uri(self->m_web_view);
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

        void WebView::on_load_changed(void*, int load_event, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return;
            const char* url = webkit_web_view_get_uri(self->m_web_view);
            LOG_INFO << "[WEB] Load status changed for " << (url ? url : "unknown") << " (Event: " << load_event << ")";
            
            // BREAK 10% STALL: Reinforce focus/visibility on EVERY load state transition
            if (self->m_backend) {
                wpe_view_backend_add_activity_state(self->m_backend, 7); // 7 = VISIBLE | FOCUSED | IN_WINDOW
                LOG_INFO << "[WEB] Focus reinforced for load event: " << load_event;
            }

            if (load_event == 2 || load_event == 3) { // COMMITTED or FINISHED
                LOG_INFO << "[WEB] Injecting diagnostic script bridge into WebView: " << self;
                const char* diag_script = 
                    "if (!window._horizon_injected) {"
                    "  const bridge = (msg) => { document.title = msg; };"
                    "  const origRF = Element.prototype.requestFullscreen || Element.prototype.webkitRequestFullscreen;"
                    "  if (origRF) {"
                    "    const wrap = function() {"
                    "      bridge('FS_LOG:requestFullscreen called on <' + this.tagName + '>');"
                    "      const p = origRF.apply(this, arguments);"
                    "      if (p && p.catch) {"
                    "        p.catch(err => bridge('FS_ERROR:' + err.name + ': ' + err.message));"
                    "      }"
                    "      return p;"
                    "    };"
                    "    Element.prototype.requestFullscreen = wrap;"
                    "    Element.prototype.webkitRequestFullscreen = wrap;"
                    "  }"
                    "  window._horizon_injected = true;"
                    "  bridge('FS_LOG:Bridge Active (Redundant)');"
                    "}";
                webkit_web_view_evaluate_javascript(self->m_web_view, diag_script, -1, NULL, NULL, NULL, NULL, NULL);
            }

            // --- UI STATUS NOTIFICATION ---
            if (self->application()) {
                bool loading = (load_event != 3); // 3 = WEBKIT_LOAD_FINISHED
                self->application()->post_task([self, loading]() mutable {
                    self->when_loading_changed.run(loading);
                });
            }
        }

        void WebView::on_progress_notify(void*, void*, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self || !self->application()) return;
            
            double progress = webkit_web_view_get_estimated_load_progress(self->m_web_view);
            
            self->application()->post_task([self, progress]() mutable {
                self->when_progress_changed.run(progress);
            });
        }

        void WebView::on_mouse_target_changed(void*, void*, uint32_t, void*) {}
        
        int WebView::on_enter_fullscreen(void* view, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return 1;

            LOG_INFO << "[WEB] enter-fullscreen signal received (is_fs=" << self->m_is_fullscreen << ")";
            
            // ATOMIC CHECK: Only act if we aren't already transitioning to the same state
            if (self->m_is_fullscreen && !self->m_waiting_for_native_frame) {
                LOG_INFO << "[WEB] enter-fullscreen ignored (already FS)";
                return 1; 
            }

            self->m_is_fullscreen = true;
            self->m_pending_fullscreen_ack = true;
            self->m_last_fullscreen_time = std::chrono::steady_clock::now();
            self->m_waiting_for_native_frame = true;

            // FORCED BACKEND SYNC: Don't wait for compositor, tell WPE the truth now.
            if (self->m_backend) {
                wpe_view_backend_dispatch_set_size(self->m_backend, 1920, 1080);
            }
            
            if (self->application()) {
                self->application()->post_task([self]() {
                    bool fs = true;
                    self->when_fullscreen_changed.run(fs);
                });
                
                // CRITICAL: We only request physical FS once per transition.
                self->application()->fullscreen();
            }

            // ACTIVATE TRANSPARENCY SHIM & FORCE IMMEDIATE LAYOUT
            self->calculate_layout();
            webkit_web_view_evaluate_javascript(self->m_web_view, 
                "document.documentElement.classList.add('horizon-fs'); document.body.classList.add('horizon-fs'); window.dispatchEvent(new Event('resize'));",
                -1, NULL, NULL, NULL, NULL, NULL);

            return 1; // Handled
        }
        
        int WebView::on_leave_fullscreen(void* view, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return 1;

            LOG_INFO << "[WEB] leave-fullscreen signal received";
            
            self->m_is_fullscreen = false;
            self->m_pending_fullscreen_ack = false; // CRITICAL: Reset, don't arm
            self->m_waiting_for_native_frame = false;
            
            // RESET BACKEND IMMEDIATELY to prevent re-entry flicker
            if (self->m_backend) {
                wpe_view_backend_dispatch_set_size(self->m_backend, 1024, 768);
            }

            // UNFREEZE IMMEDIATELY
            self->m_last_fullscreen_time = std::chrono::steady_clock::now() - std::chrono::seconds(10);

            // RESTORE REALITY: V8.5 (Persistent State)
            const char* restoration_js = 
                "document.title = 'YouTube';"
                "document.documentElement.classList.remove('horizon-fs');"
                "document.body.classList.remove('horizon-fs');"
                "window.dispatchEvent(new Event('resize'));"
                "document.dispatchEvent(new Event('fullscreenchange'));";

            webkit_web_view_evaluate_javascript(self->m_web_view, restoration_js, -1, NULL, NULL, NULL, NULL, NULL);

            if (self->application()) {
                self->application()->post_task([self]() {
                    bool fs = false;
                    self->when_fullscreen_changed.run(fs);
                });
                self->application()->unfullscreen();
            }
            return 1; // Handled
        }

        int WebView::on_permission_request(void*, void* request, void*) {
            const char* type_name = G_OBJECT_TYPE_NAME(request);
            LOG_INFO << "[WEB] Permission request received: " << (type_name ? type_name : "unknown");
            
            webkit_permission_request_allow((WebKitPermissionRequest*)request);
            return 1; // TRUE
        }

        int WebView::on_decide_policy(void* view, void* decision, int type, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return 0;

            LOG_INFO << "[WEB-POLICY] Decision requested. Type: " << type << " for WebView: " << self;

            // PERSISTENT NAVIGATION SHIELD (5 Seconds)
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - self->m_last_fullscreen_time).count();
            
            // Type 0 is WEBKIT_POLICY_DECISION_TYPE_NAVIGATION
            if (self->m_is_fullscreen && elapsed < 5 && type == 0) {
                LOG_INFO << "[WEB-SHIELD] TOTAL NAVIGATION LOCKOUT (" << elapsed << "s): Blocking transition to avoid exit.";
                webkit_policy_decision_ignore(WEBKIT_POLICY_DECISION(decision));
                return 1; // Handled
            }

            if (type == 0) {
                WebKitNavigationPolicyDecision* nav_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
                WebKitNavigationAction* action = webkit_navigation_policy_decision_get_navigation_action(nav_decision);
                WebKitURIRequest* request = webkit_navigation_action_get_request(action);
                const char* uri = webkit_uri_request_get_uri(request);
                
                // NOVA URI BRIDGE: Intercept exit signal
                if (uri && g_str_has_prefix(uri, "nova://exit_fullscreen")) {
                    LOG_INFO << "[WEB-BRIDGE] Exit signal detected via URI. Triggering Leave FS.";
                    on_leave_fullscreen(NULL, self);
                    webkit_policy_decision_ignore(WEBKIT_POLICY_DECISION(decision));
                    return 1;
                }

                LOG_INFO << "[WEB-DEBUG] Policy Navigation allowed: is_fs=" << self->m_is_fullscreen << ", elapsed=" << elapsed << "s, uri=" << (uri ? uri : "unknown");
            }
            if (self && self->m_backend) {
                // Using 7 = VISIBLE | FOCUSED | IN_WINDOW
                wpe_view_backend_add_activity_state(self->m_backend, 7);
            }
            
            // DEFAULT BACKGROUND: Ensure a solid white background
            WebKitColor background_color;
            background_color.red = 1; background_color.green = 1;
            background_color.blue = 1; background_color.alpha = 1;
            webkit_web_view_set_background_color(self->m_web_view, &background_color);

            webkit_policy_decision_use((WebKitPolicyDecision*)decision);
            return 1; // TRUE
        }
        
        int WebView::on_context_menu(void* web_view, void* context_menu, void* event, void* hit_test_result, void* self) {
            return 1; // TRUE = Handled, prevents native menu
        }

    

        std::string WebView::get_title() const {
             std::lock_guard<std::mutex> lock(m_metadata_mutex);
             return m_cached_title;
        }

        std::string WebView::get_url() const {
             std::lock_guard<std::mutex> lock(m_metadata_mutex);
             return m_cached_url;
        }

        void WebView::on_frame_exported(void *data, struct wpe_fdo_shm_exported_buffer *buffer)
        {
            auto *self = static_cast<WebView *>(data);
            if (!self || !self->m_exportable) return;

            static int frame_count = 0;
            if (++frame_count % 30 == 0) {
                LOG_INFO << "[WEB-DEBUG] Frame exported: #" << frame_count;
            }

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

            // --- FRAME SYNCHRONIZATION (THE COG METHOD) ---
            if (self->m_waiting_for_native_frame) {
                bool matched = false;
                if (self->m_is_fullscreen) {
                    int target_w = self->application() ? self->application()->width() : 1920;
                    int target_h = self->application() ? self->application()->height() : 1080;
                    matched = (width == target_w && height == target_h);
                } else {
                    // When leaving, we wait for a frame that matches our current widget size
                    matched = (width == self->m_last_dispatched_width && height == self->m_last_dispatched_height);
                }

                if (matched) {
                    LOG_INFO << "[WEB] Buffer sync matched (" << width << "x" << height << "). Dispatched " 
                             << (self->m_is_fullscreen ? "Fullscreen" : "Windowed") << " ACK.";
                    self->m_waiting_for_native_frame = false;
                    
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        WebView* self = static_cast<WebView*>(data);
                        if (self && self->m_backend) {
                            if (self->m_is_fullscreen) {
                                // REINFORCE FOCUS: level 7 = VISIBLE | FOCUSED | IN_WINDOW
                                wpe_view_backend_add_activity_state(self->m_backend, 7);
                                wpe_view_backend_dispatch_did_enter_fullscreen(self->m_backend);
                            } else {
                                wpe_view_backend_dispatch_did_exit_fullscreen(self->m_backend);
                            }
                            
                            if (self->application()) {
                                self->application()->post_task([self]() {
                                // Reinforce FS geometry in JS
                                char fs_js[512];
                                snprintf(fs_js, sizeof(fs_js), 
                                    "window.dispatchEvent(new Event('resize'));"
                                    "if(window.screen) { try { Object.defineProperty(screen, 'width', { value: %d, configurable: true }); Object.defineProperty(screen, 'height', { value: %d, configurable: true }); } catch(e) {} }",
                                    self->application() ? self->application()->width() : 1920, 
                                    self->application() ? self->application()->height() : 1080);
                                webkit_web_view_evaluate_javascript(self->m_web_view, fs_js, -1, NULL, NULL, NULL, NULL, NULL);
                                });
                            }
                        }
                        return FALSE;
                    }, self);
                }
            }

            wpe_view_backend_exportable_fdo_dispatch_release_shm_exported_buffer(self->m_exportable, buffer);
            wpe_view_backend_exportable_fdo_dispatch_frame_complete(self->m_exportable);
            
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

        void WebView::update_scrollbars()
        {
            if (m_is_fullscreen) return; 
            
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
    
        void WebView::handle_ui_scroll(int x, int y) {
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

        void WebView::load_url(const std::string &url)
        {
            if (!m_initialized) init_wpe();

            // LOAD GATE: Wayfire requires window activation before rendering starts
            if (!m_window_activated) {
                LOG_INFO << "[WEB] Load Gate engaged. Queuing URL: " << url;
                m_pending_url = url;
                return;
            }

            if (s_worker_context) {
                g_main_context_invoke_full(s_worker_context, G_PRIORITY_HIGH, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* pair = static_cast<std::pair<WebView*, std::string>*>(data);
                    if (pair->first->m_web_view) {
                        LOG_INFO << "Loading URI (High Priority): " << pair->second << " in WebView: " << pair->first;
                        webkit_web_view_load_uri(pair->first->m_web_view, pair->second.c_str());
                    } else {
                        LOG_ERROR << "Failed to load URI: m_web_view is NULL for WebView: " << pair->first;
                    }
                    delete pair;
                    return FALSE;
                }, new std::pair<WebView*, std::string>(this, url), NULL);
            }
        }

        void WebView::draw(GraphicsContext &ctx)
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
            if (!m_is_fullscreen && m_show_v_scroll) {
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

        void WebView::calculate_layout()
        {
            // LOAD GATE: Release the pending URL once we are active and have dimensions
            if (!m_window_activated && is_effectively_visible() && width() > 100) {
                m_window_activated = true; 
                if (!m_pending_url.empty()) {
                    std::string url = m_pending_url;
                    m_pending_url = "";
                    LOG_INFO << "[WEB] Load Gate released. Processing queued URL: " << url;
                    load_url(url);
                }
            }
            if (m_is_fullscreen) {
                set_position(0, 0);
                if (application()) {
                    set_size(application()->width(), application()->height());
                }
            }

            Widget::calculate_layout();

            if (m_initialized && m_backend && width() > 0 && height() > 0 && s_worker_context)
            {
                // Never return early during fullscreen transitions to ensure backend sync
                if (!m_is_fullscreen && width() == m_last_dispatched_width && height() == m_last_dispatched_height) return;
                
                int old_w = m_last_dispatched_width;
                int old_h = m_last_dispatched_height;
                m_last_dispatched_width = width();
                m_last_dispatched_height = height();

                // LAYOUT FREEZE: Only block events when ENTERING fullscreen to avoid compositor noise.
                // We do NOT block when leaving, to allow Nova to restore its UI.
                if (m_is_fullscreen && m_waiting_for_native_frame) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_fullscreen_time).count();
                    if (elapsed_ms < 2000) {
                        LOG_INFO << "[WEB-SHIELD] Layout Freeze ACTIVE (" << elapsed_ms << "ms): Ignoring late Configure event during ENTRY.";
                        return;
                    }
                }

                struct ResizeData {
                    WebView* self;
                    struct wpe_view_backend* backend;
                    int w;
                    int h;
                };

                // GEOMETRIC HARD-OVERRIDE: If in fullscreen, we MUST be exactly 1920x1080.
                // This ignores any small margins or borders Nova's layout might have.
                int target_w = (m_is_fullscreen && application()) ? application()->width() : width();
                int target_h = (m_is_fullscreen && application()) ? application()->height() : height();

                auto* rd = new ResizeData{this, m_backend, target_w, target_h};
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* d = static_cast<ResizeData*>(data);
                    
                    if (d->backend) {
                        wpe_view_backend_dispatch_set_size(d->backend, d->w, d->h);
                        LOG_INFO << "[WEB] Resize dispatched (forced=" << d->self->m_is_fullscreen << "): " << d->w << "x" << d->h;
                    }
                    
                    if (d->self->m_pending_fullscreen_ack) {
                        // Always notify platform of current expected state
                        wpe_view_backend_platform_set_fullscreen(d->backend, d->self->m_is_fullscreen);
                        
                        bool target_satisfied = false;
                        
                        if (d->self->m_is_fullscreen) {
                            int tw = d->self->application() ? d->self->application()->width() : 1920;
                            int th = d->self->application() ? d->self->application()->height() : 1080;
                            if (d->w == tw && d->h == th) {
                                LOG_INFO << "[WEB] Native " << d->w << "x" << d->h << " detected. Arming ENTER Frame Synchronization...";
                                target_satisfied = true;
                            }
                        } else {
                            int tw = d->self->application() ? d->self->application()->width() : 1920;
                            int th = d->self->application() ? d->self->application()->height() : 1080;
                            if (d->w != tw || d->h != th) {
                                LOG_INFO << "[WEB] Windowed size detected (" << d->w << "x" << d->h << "). Arming LEAVE Frame Synchronization...";
                                target_satisfied = true;
                            }
                        }

                        if (target_satisfied) {
                            d->self->m_pending_fullscreen_ack = false;
                            d->self->m_waiting_for_native_frame = true;
                        } else {
                            int tw = d->self->application() ? d->self->application()->width() : 1920;
                            int th = d->self->application() ? d->self->application()->height() : 1080;
                            LOG_INFO << "[WEB] Waiting for resolution matching state (" << (d->self->m_is_fullscreen ? std::to_string(tw) + "x" + std::to_string(th) : "windowed") << ")...";
                        }
                    }
                    
                    delete d;
                    return FALSE;
                }, rd);
            }
            else if (!m_initialized && width() > 0 && height() > 0)
            {
                init_wpe();
            }
        }

        void WebView::reload() { 
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebView*>(data);
                    if (self->m_web_view) webkit_web_view_reload(self->m_web_view);
                    return FALSE;
                }, this);
            }
        }
        void WebView::stop_loading() { 
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebView*>(data);
                    if (self->m_web_view) webkit_web_view_stop_loading(self->m_web_view);
                    return FALSE;
                }, this);
            }
        }
        void WebView::go_back() { 
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebView*>(data);
                    if (self->m_web_view) webkit_web_view_go_back(self->m_web_view);
                    return FALSE;
                }, this);
            }
        }
        void WebView::go_forward() { 
            if (s_worker_context) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebView*>(data);
                    if (self->m_web_view) webkit_web_view_go_forward(self->m_web_view);
                    return FALSE;
                }, this);
            }
        }

        bool WebView::can_go_back() const { return m_web_view && webkit_web_view_can_go_back(m_web_view); }
        bool WebView::can_go_forward() const { return m_web_view && webkit_web_view_can_go_forward(m_web_view); }

    } // namespace web
} // namespace horizon
