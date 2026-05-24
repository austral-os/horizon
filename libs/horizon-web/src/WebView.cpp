#include "horizon/web/WebView.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/Logger.hpp"
#include "horizon/WaylandWindow.hpp"
#include "horizon/ThemeManager.hpp"
#include <cstring>
#include <wayland-server-core.h>
#include "horizon/Menu.hpp"
#include "horizon/MenuItem.hpp"
#include <jsc/jsc.h>
#include <wpe/webkit.h>
#include <wpe/fdo.h>
#include <wpe/unstable/fdo-shm.h>
#include <glib.h>
#include <thread>
#include <mutex>
#include <cmath>
#include <condition_variable>


namespace horizon
{
    namespace web
    {
        // Helper to map Horizon modifiers to WPE modifiers
        static uint32_t map_horizon_to_wpe_modifiers(uint32_t mods, uint32_t active_button = 0)
        {
            uint32_t wpe_mods = 0;
            // Native horizon bits (Standard mapping)
            if (mods & 0x1) wpe_mods |= wpe_input_keyboard_modifier_shift;
            if (mods & 0x2) wpe_mods |= wpe_input_keyboard_modifier_control;
            if (mods & 0x4) wpe_mods |= wpe_input_keyboard_modifier_alt;
            if (mods & 0x8) wpe_mods |= wpe_input_keyboard_modifier_meta;
            if (active_button == 272) wpe_mods |= (1 << 8) | wpe_input_pointer_modifier_button1;
            if (active_button == 273) wpe_mods |= (1 << 9) | wpe_input_pointer_modifier_button2;
            if (active_button == 274) wpe_mods |= (1 << 10) | wpe_input_pointer_modifier_button3;

            return wpe_mods;
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
        std::atomic<GMainContext*> WebView::s_worker_context{nullptr};
        std::atomic<GMainLoop*> WebView::s_worker_loop{nullptr};
        std::atomic<bool> WebView::s_worker_running{false};
        std::mutex WebView::s_worker_mutex;
        std::condition_variable WebView::s_worker_cond;
        WebKitWebContext* WebView::s_default_context = nullptr;
        WebKitNetworkSession* WebView::s_default_session = nullptr;
        std::string WebView::s_data_directory;
        std::string WebView::s_cache_directory;

        static bool s_wpe_initialized = false;
        static std::mutex s_wpe_init_mutex;
        static bool s_gpu_enabled = true;

        static int s_instance_count = 0;
        static std::mutex s_instance_mutex;
        
        static uint32_t get_wpe_timestamp() {
            static std::atomic<uint32_t> s_last_time{0};
            uint32_t now = (uint32_t)(g_get_monotonic_time() / 1000);
            uint32_t last = s_last_time.load();
            uint32_t next;
            do {
                next = (now > last) ? now : last + 1;
            } while (!s_last_time.compare_exchange_weak(last, next));
            return next;
        }
        bool WebView::s_gpu_enabled = false;

        WebView::WebView()
        {
            set_focusable(true);
            set_background_color(theme_manager()->get_color("textbox_bg"));
            m_show_v_scroll = false;
            m_show_h_scroll = false;
            m_is_dragging_v = false;
            m_is_dragging_h = false;
            m_active_button = 0;
            m_alive_flag = std::make_shared<bool>(true);

            // Initialization is now managed by a shared static method called from the worker thread

            // Mouse Press
            when_mouse_press.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    set_focus(true);
                    m_active_button = ctx.button;

                    // Consolidated Scrollbar Check
                    if (m_show_v_scroll && ctx.x >= m_v_track_x && ctx.x < m_v_track_x + m_v_track_w &&
                        ctx.y >= m_v_track_y && ctx.y < m_v_track_y + m_v_track_h) {
                        m_is_dragging_v = true;
                        m_drag_start_pos = ctx.y;
                        {
                            std::lock_guard<std::mutex> lock{m_scroll_mutex};
                            m_drag_start_scroll = m_scroll_y;
                        }
                        ctx.stop_propagation = true;
                        return;
                    }
                    if (m_show_h_scroll && ctx.y >= m_h_track_y && ctx.y < m_h_track_y + m_h_track_h &&
                        ctx.x >= m_h_track_x && ctx.x < m_h_track_x + m_h_track_w) {
                        m_is_dragging_h = true;
                        m_drag_start_pos = ctx.x;
                        {
                            std::lock_guard<std::mutex> lock{m_scroll_mutex};
                            m_drag_start_scroll = m_scroll_x;
                        }
                        ctx.stop_propagation = true;
                        return;
                    }

                    auto lx = (int)(ctx.x - x());
                    auto ly = (int)(ctx.y - y());
                    
                    uint32_t mods_press = map_horizon_to_wpe_modifiers(ctx.modifiers, ctx.button);

                    // Send the Actual Press
                    auto* ev_press = new wpe_input_pointer_event{wpe_input_pointer_event_type_button, get_wpe_timestamp(), lx, ly, map_to_wpe_button(ctx.button), 1, mods_press};
                    
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* pair = static_cast<std::pair<WebView*, wpe_input_pointer_event*>*>(data);
                        auto* self = pair->first;
                        if (self->m_backend) {
                            wpe_view_backend_dispatch_pointer_event(self->m_backend, pair->second);
                        }
                        delete pair->second;
                        delete pair;
                        return FALSE;
                    }, new std::pair<WebView*, wpe_input_pointer_event*>(this, ev_press));
                });

            // Mouse Release
            when_mouse_release.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    if (ctx.button == 274) return; // DISALLOW MIDDLE BUTTON
                    m_is_dragging_v = false;
                    m_active_button = 0;

                    auto wx = x();
                    auto wy = y();
                    auto lx = (int)(ctx.x - wx);
                    auto ly = (int)(ctx.y - wy);
                    auto mods = map_horizon_to_wpe_modifiers(ctx.modifiers, 0); // RELEASE: No button down
                    

                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_button,
                                                             get_wpe_timestamp(),
                                                             lx, ly,
                                                             map_to_wpe_button(ctx.button),
                                                             0,
                                                             mods};
                    ctx.stop_propagation = true;
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebView*, wpe_input_pointer_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_pointer_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebView*, wpe_input_pointer_event*>(this, event));
                });

            // Mouse Move
            // Mouse Move (Hover)
            when_mouse_move.connect(
                [this](MouseMoveEventContext &ctx)
                {

                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_motion,
                                                             get_wpe_timestamp(),
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             0,
                                                             0,
                                                             map_horizon_to_wpe_modifiers(ctx.modifiers, m_active_button)};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebView*, wpe_input_pointer_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_pointer_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebView*, wpe_input_pointer_event*>(this, event));
                });

            when_mouse_drag.connect(
                [this](MouseMoveEventContext &ctx)
                {
                    if (m_is_dragging_v) {
                        double delta_y = ctx.y - m_drag_start_pos;
                        double track_usable = m_v_track_h - std::max(20, (int)(m_v_track_h * ((double)height() / m_content_height)));
                        if (track_usable > 0) {
                            double scroll_max = m_content_height - height();
                            double new_y = m_drag_start_scroll + (delta_y * (scroll_max / track_usable));
                            handle_ui_scroll(-1, (int)new_y);
                        }
                    } else if (m_is_dragging_h) {
                        double delta_x = ctx.x - m_drag_start_pos;
                        double track_usable = m_h_track_w - std::max(20, (int)(m_h_track_w * ((double)width() / m_content_width)));
                        if (track_usable > 0) {
                            double scroll_max = m_content_width - width();
                            double new_x = m_drag_start_scroll + (delta_x * (scroll_max / track_usable));
                            handle_ui_scroll((int)new_x, -1);
                        }
                    } else {
                        auto lx = (int)(ctx.x - x());
                        auto ly = (int)(ctx.y - y());
                        uint32_t drag_mods = map_horizon_to_wpe_modifiers(ctx.modifiers, 272); 
                        
                        
                        struct DragData {
                            WebView* self;
                            wpe_input_pointer_event* event;
                            double lx, ly;
                        };

                        auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_motion,
                                                                 get_wpe_timestamp(), 
                                                                 lx, ly,
                                                                 0, 1, 
                                                                 drag_mods};
                        
                        ctx.stop_propagation = true;

                        g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                            auto* d = static_cast<DragData*>(data);
                            auto* self = d->self;
                            
                            if (self->m_web_view && self->m_backend) {
                                // 1. Dispatch Native Motion first to let the engine update its internal pointer location
                                wpe_view_backend_dispatch_pointer_event(self->m_backend, d->event);

                                // 2. Force Selection Extension via JS to overcome stuck Focus
                                char buf[1024];
                                snprintf(buf, sizeof(buf), 
                                    "{ const sel = window.getSelection();"
                                    "  if (sel && sel.anchorNode) {"
                                    "    const range = document.caretRangeFromPoint(%f, %f);"
                                    "    if (range) {"
                                    "      const node = range.startContainer;"
                                    "      const offset = range.startOffset;"
                                    "      if (node) sel.extend(node, offset);"
                                    "    }"
                                    "  } }", d->lx, d->ly);
                                webkit_web_view_evaluate_javascript(self->m_web_view, buf, -1, NULL, NULL, NULL, NULL, NULL);
                            }
                            delete d->event;
                            delete d;
                            return FALSE;
                        }, new DragData{this, event, (double)lx, (double)ly});
                    }
                });

            // Mouse Enter
            when_mouse_enter.connect(
                [this](EventContext &ctx)
                {
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* self = static_cast<WebView*>(data);
                        if (self->m_backend) {
                            wpe_view_backend_add_activity_state(self->m_backend, 7);
                        }
                        return FALSE;
                    }, this);
                });
                
            when_mouse_leave.connect([this](EventContext &ctx) {});

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
                    // We invert the sign (-8) to match the expected direction (Natural scrolling).
                    dispatch_axis(0, (int)(ctx.dy * -8)); // Vertical
                    dispatch_axis(1, (int)(ctx.dx * -8)); // Horizontal
                });

            when_application_load.connect([this](EventContext&) {
                if (application()) {
                    application()->when_popup_dismissed.connect([this](PopupDismissedContext &) {
                        m_active_context_menu.reset();
                    });
                }
            });

            when_right_click.connect([this](MouseButtonEventContext &ctx) {
                if (application()) {
                    // Use the CACHED hit test info since we are in the UI thread 
                    // and WPE might not have triggered on_context_menu
                    
                    uint32_t context = m_hit_test_cache.context;
                    std::string link_uri = m_hit_test_cache.link_uri;
                    std::string image_uri = m_hit_test_cache.image_uri;
                    
                    bool back = can_go_back();
                    bool forward = can_go_forward();
                    std::string current_url = get_url();

                    m_active_context_menu = std::make_unique<horizon::Menu>();
                    auto* m = m_active_context_menu.get();

                    // Navigation
                    auto* b = m->add_item("Atrás", "Alt+Izquierda", "back");
                    b->set_icon("go-previous-symbolic");
                    b->set_enabled(back);
                    b->when_click.connect([this](horizon::MouseButtonEventContext&) { 
                        this->go_back(); 
                        if (this->application()) this->application()->hide_context_menu(); 
                    });

                    auto* f = m->add_item("Adelante", "Alt+Derecha", "forward");
                    f->set_icon("go-next-symbolic");
                    f->set_enabled(forward);
                    f->when_click.connect([this](horizon::MouseButtonEventContext&) { 
                        this->go_forward(); 
                        if (this->application()) this->application()->hide_context_menu(); 
                    });

                    auto* r = m->add_item("Recargar", "Ctrl+R", "reload");
                    r->set_icon("view-refresh-symbolic");
                    r->when_click.connect([this](horizon::MouseButtonEventContext&) { 
                        this->reload(); 
                        if (this->application()) this->application()->hide_context_menu(); 
                    });

                    m->add_separator();

                    // Downloads
                    bool has_download = false;
                    if (!link_uri.empty()) {
                        auto* item = m->add_item("Descargar vínculo");
                        item->set_icon("folder-download-symbolic");
                        item->when_click.connect([this, link_uri](horizon::MouseButtonEventContext&) mutable {
                            this->when_download_requested.run(link_uri);
                            if (this->application()) this->application()->hide_context_menu();
                        });
                        has_download = true;
                    }

                    if (!image_uri.empty()) {
                        auto* item = m->add_item("Descargar imagen");
                        item->set_icon("image-x-generic-symbolic");
                        item->when_click.connect([this, image_uri](horizon::MouseButtonEventContext&) mutable {
                            this->when_download_requested.run(image_uri);
                            if (this->application()) this->application()->hide_context_menu();
                        });
                        has_download = true;
                    }

                    if (!has_download) {
                        auto* item = m->add_item("Descargar esta página");
                        item->set_icon("document-save-symbolic");
                        item->when_click.connect([this, current_url](horizon::MouseButtonEventContext&) mutable {
                            this->when_download_requested.run(const_cast<std::string&>(current_url));
                            if (this->application()) this->application()->hide_context_menu();
                        });
                    }

                    m->add_separator();

                    // Inspector
                    if (!m_inspector_visible) {
                        auto* inspect = m->add_item("Inspeccionar elemento");
                        inspect->set_icon("edit-find-symbolic");
                        inspect->when_click.connect([this](horizon::MouseButtonEventContext&) {
                            this->m_inspector_visible = true;
                            this->invalidate();
                            if (s_worker_context) {
                                const char* eruda_js = 
                                    "(function () { "
                                    "  if (window.eruda) { eruda.show(); } else { "
                                    "    var script = document.createElement('script'); "
                                    "    script.src = 'https://cdn.jsdelivr.net/npm/eruda'; "
                                    "    document.body.appendChild(script); "
                                    "    script.onload = function () { "
                                    "       eruda.init(); try { eruda._entryBtn.hide(); } catch(e) {} "
                                    "       var style = document.createElement('style'); "
                                    "       style.innerHTML = '.eruda-entry-btn { display: none !important; }'; "
                                    "       document.head.appendChild(style); eruda.show(); "
                                    "       setTimeout(() => { window.dispatchEvent(new Event('resize')); }, 500); "
                                    "    }; "
                                    "  } "
                                    "})();";
                                
                                struct JSData { WebKitWebView* v; std::string s; };
                                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                                    auto* d = (JSData*)data;
                                    if (d->v) webkit_web_view_evaluate_javascript(d->v, d->s.c_str(), -1, NULL, NULL, NULL, NULL, NULL);
                                    delete d;
                                    return FALSE;
                                }, new JSData{m_web_view, eruda_js});
                            }
                            if (application()) application()->hide_context_menu();
                        });
                    } else {
                        auto* hide = m->add_item("Ocultar inspector");
                        hide->set_icon("edit-find-symbolic");
                        hide->when_click.connect([this](horizon::MouseButtonEventContext&) {
                            this->m_inspector_visible = false;
                            this->invalidate();
                            if (s_worker_context) {
                                const char* eruda_hide_js = "if (window.eruda) eruda.hide();";
                                struct JSData { WebKitWebView* v; std::string s; };
                                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                                    auto* d = (JSData*)data;
                                    if (d->v) webkit_web_view_evaluate_javascript(d->v, d->s.c_str(), -1, NULL, NULL, NULL, NULL, NULL);
                                    delete d;
                                    return FALSE;
                                }, new JSData{m_web_view, eruda_hide_js});
                            }
                            if (application()) application()->hide_context_menu();
                        });
                    }

                    application()->show_context_menu(m, -1, -1, ctx.serial, this);
                    ctx.stop_propagation = true;
                }
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

            m_last_v_show_time = std::chrono::steady_clock::now();
            m_last_h_show_time = std::chrono::steady_clock::now();
        }

        WebView::~WebView()
        {
            *m_alive_flag = false;
            // Disconnect all signals
            when_title_changed.disconnect_all();
            when_url_changed.disconnect_all();
            when_loading_changed.disconnect_all();
            when_progress_changed.disconnect_all();

            GMainContext* ctx = s_worker_context.load();
            if (ctx && m_web_view) {
                struct CleanupData {
                    WebKitWebView* view;
                    std::atomic<bool>* done;
                };
                std::atomic<bool> done{false};
                auto* cd = new CleanupData{m_web_view, &done};
                
                g_main_context_invoke(ctx, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* d = static_cast<CleanupData*>(data);
                    g_object_unref(d->view);
                    d->done->store(true);
                    delete d;
                    return FALSE;
                }, cd);

                auto start = std::chrono::steady_clock::now();
                while (!done.load()) {
                    if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(200)) break;
                    std::this_thread::yield();
                }
            }

            m_web_view = nullptr;
            m_backend = nullptr;
            m_exportable = nullptr;

            if (m_cairo_surface)
            {
                std::lock_guard<std::mutex> lock(m_surface_mutex);
                cairo_surface_destroy(m_cairo_surface);
                m_cairo_surface = nullptr;
            }

            std::lock_guard<std::mutex> ilock(s_instance_mutex);
            s_instance_count--;
        }

        void WebView::init_wpe()
        {
            if (m_initialized)
                return;

            ensure_worker_thread();
            
            {
                std::lock_guard<std::mutex> ilock(s_instance_mutex);
                s_instance_count++;
            }

            g_main_context_invoke_full(s_worker_context, G_PRIORITY_HIGH, (GSourceFunc)+[](void* data) -> gboolean {
                auto* self = static_cast<WebView*>(data);
                
                self->m_client = {
                    .export_buffer_resource = nullptr,
                    .export_dmabuf_resource = [](void *data, struct wpe_view_backend_exportable_fdo_dmabuf_resource *dmabuf)
                    {
                        auto *self = static_cast<WebView *>(data);
                        wpe_view_backend_exportable_fdo_dispatch_frame_complete(self->m_exportable);
                        wpe_view_backend_exportable_fdo_dispatch_release_buffer(self->m_exportable, dmabuf->buffer_resource);
                    },
                    .export_shm_buffer = [](void *data, struct wpe_fdo_shm_exported_buffer *buffer)
                    {
                        auto *self = static_cast<WebView *>(data);
                        WebView::on_frame_exported(self, buffer);
                    },
                    ._wpe_reserved0 = reinterpret_cast<void(*)(void)>(static_cast<void(*)(void*, bool)>(WebView::on_fs_callback)),
                    ._wpe_reserved1 = nullptr};

                int initial_width = self->width() > 0 ? self->width() : 800;
                int initial_height = self->height() > 0 ? self->height() : 600;

                self->m_exportable = wpe_view_backend_exportable_fdo_create(&self->m_client, self, initial_width, initial_height);
                self->m_backend = wpe_view_backend_exportable_fdo_get_view_backend(self->m_exportable);
                
                wpe_view_backend_add_activity_state(self->m_backend, 7); 
                wpe_view_backend_platform_set_fullscreen(self->m_backend, FALSE);

                auto *webkit_backend = webkit_web_view_backend_new(self->m_backend, nullptr, nullptr);

                printf("\n[WEB] WebView System Starting (v0.1.2-inspt)\n");
                WebKitSettings* settings = webkit_settings_new();
                webkit_settings_set_enable_fullscreen(settings, TRUE);
                webkit_settings_set_javascript_can_open_windows_automatically(settings, TRUE);
                webkit_settings_set_enable_javascript_markup(settings, TRUE);
                webkit_settings_set_enable_webgl(settings, s_gpu_enabled);
                webkit_settings_set_enable_2d_canvas_acceleration(settings, s_gpu_enabled);
                webkit_settings_set_enable_developer_extras(settings, TRUE);
                webkit_settings_set_user_agent(settings, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/147.0.0.0 Safari/537.36");
                
                self->m_web_view = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
                    "backend", webkit_backend,
                    "web-context", s_default_context,
                    "network-session", s_default_session,
                    "settings", settings,
                    NULL));
                
                g_object_set_data(G_OBJECT(self->m_web_view), "horizon-wrapper", self);
                g_object_unref(settings);

                g_signal_connect(self->m_web_view, "notify::title", G_CALLBACK(on_title_notify), self);
                g_signal_connect(self->m_web_view, "notify::uri", G_CALLBACK(on_uri_notify), self);
                g_signal_connect(self->m_web_view, "notify::estimated-load-progress", G_CALLBACK(on_progress_notify), self);
                g_signal_connect(self->m_web_view, "load-changed", G_CALLBACK(on_load_changed), self);
                
                g_signal_connect(self->m_web_view, "enter-fullscreen", G_CALLBACK(WebView::on_enter_fullscreen), self);
                g_signal_connect(self->m_web_view, "leave-fullscreen", G_CALLBACK(WebView::on_leave_fullscreen), self);
                g_signal_connect(self->m_web_view, "permission-request", G_CALLBACK(WebView::on_permission_request), self);
                g_signal_connect(self->m_web_view, "decide-policy", G_CALLBACK(WebView::on_decide_policy), self);
                g_signal_connect(self->m_web_view, "context-menu", G_CALLBACK(WebView::on_context_menu), self);
                g_signal_connect(self->m_web_view, "mouse-target-changed", G_CALLBACK(WebView::on_mouse_target_changed), self);
                
                g_signal_connect(self->m_web_view, "web-process-terminated", G_CALLBACK(+[](WebKitWebView* web_view, WebKitWebProcessTerminationReason reason, WebView* self) {
                    LOG_ERROR << "[WEB-CRITICAL] Web process terminated! Reason: " << reason;
                    if (reason != WEBKIT_WEB_PROCESS_TERMINATED_BY_API) {
                        webkit_web_view_reload(web_view);
                    }
                }), self);

                WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(self->m_web_view);
                const char* isolation_script = 
                    "const curT = document.title; "
                    "window._horizon_real_title = (curT && curT.indexOf('HORIZON_') !== 0 && curT.indexOf('FS_') !== 0) ? curT : ''; "
                    "const observer = new MutationObserver(() => { "
                    "  const t = document.title; "
                    "  if (t && t.indexOf('HORIZON_') !== 0 && t.indexOf('FS_LOG:') !== 0 && t.indexOf('FS_ERROR:') !== 0) { "
                    "    window._horizon_real_title = t; "
                    "  } "
                    "}); "
                    "observer.observe(document.documentElement, { subtree: true, childList: true, characterData: true }); "
                    "const sendScroll = () => { "
                    "  const el = document.scrollingElement || document.documentElement; "
                    "  const sy = Math.round(window.scrollY || window.pageYOffset); "
                    "  const sx = Math.round(window.scrollX || window.pageXOffset); "
                    "  const sh = el.scrollHeight; "
                    "  const sw = el.scrollWidth; "
                    "  let wh = window.innerHeight; "
                    "  const ww = window.innerWidth; "
                    "  try { "
                    "    const eruda = document.getElementById('eruda'); "
                    "    if (eruda) { "
                    "      const devTools = eruda.querySelector('.eruda-dev-tools'); "
                    "      if (devTools) { "
                    "        const rect = devTools.getBoundingClientRect(); "
                    "        if (rect.height > 10 && window.getComputedStyle(devTools).display !== 'none') { "
                    "          wh -= rect.height; "
                    "        } "
                    "      } "
                    "    } "
                    "  } catch(e) {} "
                    "  const realTitle = window._horizon_real_title || document.title || ''; "
                    "  let iv = 0; "
                    "  try { "
                    "    const eruda = document.getElementById('eruda'); "
                    "    if (eruda) { "
                    "      const devTools = eruda.querySelector('.eruda-dev-tools'); "
                    "      if (devTools && window.getComputedStyle(devTools).display !== 'none') { "
                    "        iv = 1; "
                    "      } "
                    "    } "
                    "  } catch(e) {} "
                    "  if (realTitle.indexOf('HORIZON_SCROLL:') === 0) { "
                    "    document.title = 'HORIZON_SCROLL:' + sy + ' ' + sx + ' ' + sh + ' ' + sw + ' ' + wh + ' ' + ww + ' ' + iv + ' TITLE:'; "
                    "  } else { "
                    "    document.title = 'HORIZON_SCROLL:' + sy + ' ' + sx + ' ' + sh + ' ' + sw + ' ' + wh + ' ' + ww + ' ' + iv + ' TITLE:' + realTitle; "
                    "  } "
                    "}; "
                    "window.addEventListener('resize', sendScroll); "
                    "window.addEventListener('scroll', sendScroll); "
                    "setInterval(sendScroll, 1000); "
                    "let lastC = ''; "
                    "window.addEventListener('mousemove', (e) => { "
                    "  try { "
                    "    const c = window.getComputedStyle(e.target).cursor; "
                    "    if (c !== lastC) { "
                    "      lastC = c; "
                    "      document.title = 'HORIZON_CURSOR:' + c; "
                    "    } "
                    "  } catch(e) {} "
                    "}, {passive: true}); "; 
                WebKitUserScript* script = webkit_user_script_new(isolation_script, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, NULL, NULL);
                webkit_user_content_manager_add_script(manager, script);
                webkit_user_script_unref(script);

                self->m_initialized = true;
                return FALSE;
            }, this, NULL);
        }

        void WebView::shutdown() {
            std::unique_lock<std::mutex> lock(s_worker_mutex);
            if (!s_worker_running) return;

            LOG_INFO << "[WEB-DEBUG] Shutting down shared worker thread...";
            s_worker_running = false;

            GMainLoop* loop = s_worker_loop.load();
            if (loop) {
                g_main_loop_quit(loop);
            }

            if (s_worker_thread.joinable()) {
                // Unlock during join to avoid deadlocks if the thread is waiting for this mutex
                lock.unlock();
                s_worker_thread.join();
                lock.lock();
            }

            s_worker_context.store(nullptr);
            s_worker_loop.store(nullptr);
            LOG_INFO << "[WEB-DEBUG] Shared worker thread stopped.";
        }

        void WebView::ensure_worker_thread()
        {
            std::unique_lock<std::mutex> lock(s_worker_mutex);
            if (s_worker_running) return;

            s_worker_running = true;
            s_worker_thread = std::thread(&WebView::worker_thread_func);
            
            if (!s_worker_cond.wait_for(lock, std::chrono::seconds(5), []() { 
                return s_worker_context.load() != nullptr; 
            })) {
                LOG_ERROR << "[WEB-CRITICAL] Worker thread failed to signal readiness in 5s!";
            }
        }

        void WebView::worker_thread_func() 
        {
            LOG_INFO << "[WEB-DEBUG] Shared worker thread starting";
            GMainContext* ctx = g_main_context_new();
            g_main_context_push_thread_default(ctx);
            GMainLoop* loop = g_main_loop_new(ctx, FALSE);

            s_worker_context.store(ctx, std::memory_order_release);
            s_worker_loop.store(loop, std::memory_order_release);

            // Global WebKit Environment
            g_setenv("WPE_FDO_FORCE_SHM", "1", TRUE);
            g_setenv("WEBKIT_DISABLE_WAYLAND_DISPLAY_CHECK", "1", TRUE);
            g_setenv("WEBKIT_FORCE_SANDBOX", "0", TRUE);
            g_setenv("WEBKIT_DISABLE_SANDBOX_THIS_IS_DANGEROUS", "1", TRUE);
            g_setenv("WEBKIT_INSPECTOR_HTTP_SERVER_PORT", "8080", FALSE);

            {
                std::lock_guard<std::mutex> lock(s_wpe_init_mutex);
                if (!s_wpe_initialized) {
                    LOG_INFO << "Initializing WPE Backend...";
                    const char* paths[] = {
                        "/usr/lib/x86_64-linux-gnu/libWPEBackend-fdo-1.0.so.1",
                        "/usr/lib/libWPEBackend-fdo-1.0.so.1",
                        "libWPEBackend-fdo-1.0.so.1"
                    };
                    for (const char* path : paths) {
                        if (wpe_loader_init(path)) {
                            s_wpe_initialized = true;
                            break;
                        }
                    }
                    if (s_wpe_initialized) wpe_fdo_initialize_shm();
                }
            }

            // Global WebKit Context & Session
            if (!s_data_directory.empty()) {
                g_mkdir_with_parents(s_data_directory.c_str(), 0755);
                
                std::string cache_dir = s_cache_directory;
                if (cache_dir.empty()) cache_dir = s_data_directory + "/cache";
                g_mkdir_with_parents(cache_dir.c_str(), 0755);

                LOG_INFO << "[WEB] Enabling persistent storage:";
                LOG_INFO << "  - Data: " << s_data_directory;
                LOG_INFO << "  - Cache: " << cache_dir;

                s_default_session = webkit_network_session_new(s_data_directory.c_str(), cache_dir.c_str());
                
                // Get the website data manager from the session
                WebKitWebsiteDataManager* data_manager = webkit_network_session_get_website_data_manager(s_default_session);
                
                // Explicitly configure cookie manager for persistence
                WebKitCookieManager* cookie_manager = webkit_network_session_get_cookie_manager(s_default_session);
                std::string cookie_db_path = s_data_directory + "/cookies.db";
                webkit_cookie_manager_set_persistent_storage(cookie_manager, 
                                                            cookie_db_path.c_str(), 
                                                            WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
                webkit_cookie_manager_set_accept_policy(cookie_manager, WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);

                // Create the web context
                s_default_context = webkit_web_context_new();
            } else {
                LOG_INFO << "[WEB] No persistence directory set. Using ephemeral session.";
                s_default_session = webkit_network_session_new_ephemeral();
                s_default_context = webkit_web_context_new();
            }

            webkit_web_context_set_cache_model(s_default_context, WEBKIT_CACHE_MODEL_WEB_BROWSER);

            {
                std::lock_guard<std::mutex> lock(s_worker_mutex);
                s_worker_cond.notify_all();
            }

            LOG_INFO << "[WEB-DEBUG] Entering shared worker loop";
            g_main_loop_run(loop);
            
            if (s_default_context) {
                g_object_unref(s_default_context);
                s_default_context = nullptr;
            }
            if (s_default_session) {
                g_object_unref(s_default_session);
                s_default_session = nullptr;
            }

            g_main_context_pop_thread_default(ctx);
            g_main_loop_unref(loop);
            g_main_context_unref(ctx);
        }


        static std::string sanitize_title(const std::string& title) {
            std::string clean;
            for (unsigned char c : title) {
                // Keep only printable ASCII (32-126)
                if (c >= 32 && c <= 126) {
                    clean += c;
                }
                if (clean.length() >= 256) break;
            }
            return clean.empty() ? "Untitled" : clean;
        }

        void WebView::on_fs_callback(void* data, bool fullscreen) {
            auto* self = static_cast<WebView*>(data);
            if (!self) return;
            auto alive = self->m_alive_flag;
            if (self->application()) {
                self->application()->post_task([self, alive, fullscreen]() {
                    if (!*alive) return;
                    bool fs = fullscreen;
                    self->when_fullscreen_changed.run(fs);
                });
            }
        }

        void WebView::on_title_notify(void*, void*, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return;
            const char* title_str = webkit_web_view_get_title(self->m_web_view);
            if (!title_str) return;
            std::string title = title_str;

            // BEACON HANDLING: Intercept scroll updates masked as title changes
            if (title.find("HORIZON_SCROLL:") == 0) {
                double sy, sx, ch, cw, vh, vw;
                int iv = 0;
                if (sscanf(title.c_str() + 15, "%lf %lf %lf %lf %lf %lf %d", &sy, &sx, &ch, &cw, &vh, &vw, &iv) == 7) {
                    {
                        std::lock_guard<std::mutex> lock{self->m_scroll_mutex};
                        self->m_target_scroll_y = sy;
                        self->m_target_scroll_x = sx;
                        self->m_target_content_h = ch;
                        self->m_target_content_w = cw;
                        self->m_target_view_h = vh;
                        self->m_target_view_w = vw;
                        self->m_inspector_visible = (iv != 0);
                        self->m_scroll_dirty = true;
                        LOG_INFO << "[WEB-DEBUG] Viewport Update - VH: " << vh << " Inspector: " << (iv ? "YES" : "NO");
                    }

                    // Extract embedded real title if present
                    size_t title_pos = title.find(" TITLE:");
                    if (title_pos != std::string::npos) {
                        std::string extracted_title = title.substr(title_pos + 7);
                        // Filter out other beacon types that might have been caught as "real" titles
                        bool is_beacon = (extracted_title.find("HORIZON_") == 0 || 
                                          extracted_title.find("FS_LOG:") == 0 || 
                                          extracted_title.find("FS_ERROR:") == 0);

                        if (!extracted_title.empty() && !is_beacon) {
                            std::string real_title = sanitize_title(extracted_title);
                            std::lock_guard<std::mutex> mlock(self->m_metadata_mutex);
                            if (self->m_cached_title != real_title) {
                                LOG_INFO << "[WEB-TITLE] Syncing real title from beacon: " << real_title;
                                self->m_cached_title = real_title;
                                if (self->application()) {
                                    self->application()->post_task([self, real_title]() {
                                        auto t = real_title;
                                        self->when_title_changed.run(t);
                                    });
                                }
                            }
                        }
                    }

                    if (self->application()) {
                        auto alive = self->m_alive_flag;
                        self->application()->post_task([self, alive]() {
                            if (*alive) self->invalidate();
                        });
                    }
                    return;
                }
            }
            
            LOG_INFO << "[WEB-TITLE] Raw title update: " << title;
            
            // BEACON HANDLING: Intercept clipboard updates masked as title changes
            if (title.find("HORIZON_CLIPBOARD:") == 0) {
                std::string content = title.substr(18);
                if (!content.empty()) {
                    self->m_clipboard_content = content;
                    if (self->application()) {
                        self->application()->set_clipboard_owner(self);
                    }
                }
                return;
            }

            if (title.find("HORIZON_CURSOR:") == 0) {
                std::string c = title.substr(15);
                CursorType type = CursorType::Default;
                if (c == "pointer") type = CursorType::Pointer;
                else if (c == "text" || c == "vertical-text") type = CursorType::Text;
                else if (c == "ns-resize" || c == "n-resize" || c == "s-resize" || c == "row-resize") type = CursorType::ResizeNS;
                else if (c == "ew-resize" || c == "e-resize" || c == "w-resize" || c == "col-resize") type = CursorType::ResizeEW;
                else if (c == "nesw-resize") type = CursorType::ResizeNESW;
                else if (c == "nwse-resize") type = CursorType::ResizeNWSE;
                else if (c == "move") type = CursorType::Move;
                else if (c == "wait" || c == "progress") type = CursorType::Wait;
                else if (c == "help") type = CursorType::Help;
                else if (c == "crosshair") type = CursorType::Text; // Fallback
                
                if (self->application()) {
                    self->application()->post_task([self, type]() {
                        if (self->cursor_type() != type) {
                            self->set_cursor_type(type);
                        }
                    });
                }
                return;
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
                self->m_cached_title = sanitize_title(title);
            }

            if (self->application()) {
                std::string t = self->m_cached_title;
                self->application()->post_task([self, t]() mutable {
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
                self->application()->post_task([self, url]() mutable {
                    self->when_url_changed.run(url);
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
            }

            if (load_event == 2 || load_event == 3) { // COMMITTED or FINISHED
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

        void WebView::on_mouse_target_changed(void*, void* hit_test_result, uint32_t, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return;

            WebKitHitTestResult* result = WEBKIT_HIT_TEST_RESULT(hit_test_result);
            uint32_t context = webkit_hit_test_result_get_context(result);

            // Update Cache
            self->m_hit_test_cache.context = context;
            self->m_hit_test_cache.link_uri = "";
            self->m_hit_test_cache.image_uri = "";
            
            if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK) {
                const char* uri = webkit_hit_test_result_get_link_uri(result);
                if (uri) self->m_hit_test_cache.link_uri = uri;
            }
            if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE) {
                const char* uri = webkit_hit_test_result_get_image_uri(result);
                if (uri) self->m_hit_test_cache.image_uri = uri;
            }

            CursorType cursor = CursorType::Default;

            if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK) {
                cursor = CursorType::Pointer;
            } else if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE) {
                cursor = CursorType::Text;
            } else if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_SELECTION) {
                cursor = CursorType::Text;
            }

            if (self->application()) {
                self->application()->post_task([self, cursor]() {
                    if (self->cursor_type() != cursor) {
                        self->set_cursor_type(cursor);
                    }
                });
            }
        }
        
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

        void WebView::on_download_started(void* session, void* download, void* p_self) {
            WebKitWebView* web_view = webkit_download_get_web_view(WEBKIT_DOWNLOAD(download));
            if (!web_view) return;
            
            WebView* self = static_cast<WebView*>(g_object_get_data(G_OBJECT(web_view), "horizon-wrapper"));
            if (!self) return;

            WebKitURIRequest* request = webkit_download_get_request(WEBKIT_DOWNLOAD(download));
            const char* uri = webkit_uri_request_get_uri(request);
            
            LOG_INFO << "[WEB] Download started for URI: " << (uri ? uri : "unknown");
            
            if (uri) {
                // Cancel WebKit's internal download, we will handle it with our libhorizon-download
                webkit_download_cancel(WEBKIT_DOWNLOAD(download));
                
                // Emit signal so the application (Nova) can pick it up
                std::string s_uri = uri;
                self->when_download_requested.run(s_uri);
            }
        }

        static bool is_uri_binary(const char* uri) {
            if (!uri) return false;
            std::string s_uri = uri;
            // Remove fragments
            size_t frag_pos = s_uri.find('#');
            if (frag_pos != std::string::npos) s_uri = s_uri.substr(0, frag_pos);
            // Remove query
            size_t query_pos = s_uri.find('?');
            if (query_pos != std::string::npos) s_uri = s_uri.substr(0, query_pos);
            
            const char* binary_exts[] = {
                ".deb", ".zip", ".tar.gz", ".tar.xz", ".iso", ".bin", 
                ".exe", ".msi", ".7z", ".rar", ".gz", ".bz2", ".xz", ".pkg", ".rpm", ".appimage"
            };
            
            for (const char* ext : binary_exts) {
                if (g_str_has_suffix(s_uri.c_str(), ext)) return true;
            }
            return false;
        }

        int WebView::on_decide_policy(void* view, void* decision, int type, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return 0;


            // PERSISTENT NAVIGATION SHIELD (5 Seconds)
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - self->m_last_fullscreen_time).count();
            
            // Type 0 is WEBKIT_POLICY_DECISION_TYPE_NAVIGATION
            if (self->m_is_fullscreen && elapsed < 5 && type == 0) {
                LOG_INFO << "[WEB-SHIELD] TOTAL NAVIGATION LOCKOUT (" << elapsed << "s): Blocking transition to avoid exit.";
                webkit_policy_decision_ignore(WEBKIT_POLICY_DECISION(decision));
                return 1; // Handled
            }
            // WEBKIT_POLICY_DECISION_TYPE_NAVIGATION = 0
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
            }
            
            // WEBKIT_POLICY_DECISION_TYPE_RESPONSE = 2
            if (type == 2) {
                WebKitResponsePolicyDecision* resp_decision = WEBKIT_RESPONSE_POLICY_DECISION(decision);
                WebKitURIResponse* response = webkit_response_policy_decision_get_response(resp_decision);
                const char* uri = webkit_uri_response_get_uri(response);
                const char* mime = webkit_uri_response_get_mime_type(response);
                
                bool should_download = true;

                // Rule 0: Default to DESCARGAR on ambiguity
                bool download = true; 

                if (mime) {
                    std::string smime = mime;
                    
                    // Rule 2: SHOW (MOSTRAR)
                    if (smime == "text/html" || smime == "text/plain" || smime == "text/css" ||
                        smime == "application/javascript" || smime == "application/json" ||
                        smime == "application/pdf" ||
                        g_str_has_prefix(mime, "image/") ||
                        g_str_has_prefix(mime, "audio/") ||
                        g_str_has_prefix(mime, "video/")) {
                        
                        download = false;
                    }
                    
                    // Rule 2: Explicit DOWNLOAD (DESCARGAR) - Overrides previous SHOW check for safety
                    if (strstr(mime, "application/octet-stream") ||
                        strstr(mime, "zip") || strstr(mime, "rar") || strstr(mime, "7z") ||
                        strstr(mime, "compressed") || strstr(mime, "archive") ||
                        strstr(mime, "tar") || strstr(mime, "binary")) {
                        
                        download = true;
                    }
                }

                // Rule: If WebKit doesn't support it, we MUST download
                if (!webkit_response_policy_decision_is_mime_type_supported(resp_decision)) {
                    download = true;
                }

                if (download) {
                    LOG_INFO << "[HTTP-DECISION] DESCARGAR: " << (uri ? uri : "unknown") << " [" << (mime ? mime : "unknown") << "]";
                    if (uri && self && self->application()) {
                        std::string s_uri = uri;
                        self->application()->post_task([self, s_uri]() mutable {
                            self->when_download_requested.run(s_uri);
                        });
                        webkit_policy_decision_ignore(WEBKIT_POLICY_DECISION(decision));
                    } else {
                        webkit_policy_decision_download(WEBKIT_POLICY_DECISION(decision));
                    }
                    return 1;
                } else {
                    LOG_INFO << "[HTTP-DECISION] MOSTRAR: " << (mime ? mime : "unknown");
                    webkit_policy_decision_use(WEBKIT_POLICY_DECISION(decision));
                    return 1;
                }
            }


            if (self && self->m_backend) {
                // Using 7 = VISIBLE | FOCUSED | IN_WINDOW
                wpe_view_backend_add_activity_state(self->m_backend, 7);
            }
            
            // DEFAULT BACKGROUND: Ensure background matches textbox_bg
            Color theme_bg = theme_manager()->get_color("textbox_bg");
            WebKitColor background_color;
            background_color.red = theme_bg.r; background_color.green = theme_bg.g;
            background_color.blue = theme_bg.b; background_color.alpha = theme_bg.a;
            webkit_web_view_set_background_color(self->m_web_view, &background_color);

            webkit_policy_decision_use((WebKitPolicyDecision*)decision);
            return 1; // TRUE
        }
        
        int WebView::on_context_menu(void* web_view, void* context_menu, void* event, void* hit_test_result, void* p_self) {
            WebView* self = static_cast<WebView*>(p_self);
            if (!self) return 0;
            
            LOG_INFO << "[WEB-CONTEXT] Signal received (using fallback for UI)";
            return 1; // Handled
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

            // PREVENT FLOODING: If we are still waiting for the UI thread to draw 
            // the last frame, drop this one. This prevents background tabs from 
            // hogging the mutex and the worker context.
            if (self->m_pending_ack.load()) {
                wpe_view_backend_exportable_fdo_dispatch_release_shm_exported_buffer(self->m_exportable, buffer);
                return;
            }

            // VSYNC SYNC: We handle the buffer release but DELAY the frame_complete
            // until the UI thread has actually drawn this frame.
            // This forces WPEWebProcess to throttle its rendering to our refresh rate.

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

            // 0. If NOT visible, just ACK and bail. No expensive copy/swap needed!
            if (!self->is_effectively_visible()) {
                g_main_context_invoke(self->s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    WebView* self = static_cast<WebView*>(data);
                    if (self && self->m_exportable) {
                        wpe_view_backend_exportable_fdo_dispatch_frame_complete(self->m_exportable);
                    }
                    return FALSE;
                }, self);
                wpe_view_backend_exportable_fdo_dispatch_release_shm_exported_buffer(self->m_exportable, buffer);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(self->m_surface_mutex);
                
                // 1. Ensure BACK buffer is ready
                if (!self->m_cairo_surface_back ||
                    cairo_image_surface_get_width(self->m_cairo_surface_back) != width ||
                    cairo_image_surface_get_height(self->m_cairo_surface_back) != height)
                {
                    if (self->m_cairo_surface_back) cairo_surface_destroy(self->m_cairo_surface_back);
                    self->m_cairo_surface_back = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
                }

                // 2. Copy to BACK buffer
                unsigned char *dest = cairo_image_surface_get_data(self->m_cairo_surface_back);
                int dest_stride = cairo_image_surface_get_stride(self->m_cairo_surface_back);
                if (dest && buffer_data) {
                    cairo_surface_flush(self->m_cairo_surface_back);
                    if (dest_stride == stride) {
                        std::memcpy(dest, buffer_data, stride * height);
                    } else {
                        for (int i = 0; i < height; i++) {
                            std::memcpy(dest + i * dest_stride, (unsigned char*)buffer_data + i * stride, std::min(stride, dest_stride));
                        }
                    }
                    cairo_surface_mark_dirty(self->m_cairo_surface_back);
                }

                // 3. SWAP buffers
                std::swap(self->m_cairo_surface_front, self->m_cairo_surface_back);
                self->m_has_front_buffer.store(true);
            }

            // 4. ACK immediately to keep WebKit flowing (Double Buffering allows this!)
            g_main_context_invoke(self->s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                WebView* self = static_cast<WebView*>(data);
                if (self && self->m_exportable) {
                    wpe_view_backend_exportable_fdo_dispatch_frame_complete(self->m_exportable);
                }
                return FALSE;
            }, self);

            // 5. Invalidate UI if visible
            if (self->application() && self->is_effectively_visible()) {
                if (!self->m_waiting_for_draw.exchange(true)) {
                    auto alive = self->m_alive_flag;
                    self->application()->post_task([self, alive]() {
                        if (!*alive) return;
                        self->invalidate();
                    });
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
                    matched = (width == self->m_last_dispatched_width && height == self->m_last_dispatched_height);
                }

                if (matched) {
                    self->m_waiting_for_native_frame = false;
                    
                    g_main_context_invoke(self->s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        WebView* self = static_cast<WebView*>(data);
                        if (self && self->m_backend) {
                            if (self->m_is_fullscreen) {
                                wpe_view_backend_add_activity_state(self->m_backend, 7);
                                wpe_view_backend_dispatch_did_enter_fullscreen(self->m_backend);
                            } else {
                                wpe_view_backend_dispatch_did_exit_fullscreen(self->m_backend);
                            }
                            
                            auto alive = self->m_alive_flag;
                            if (self->application()) {
                                self->application()->post_task([self, alive]() {
                                    if (!*alive) return;
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
            // WE DO NOT CALL frame_complete here. It will be called from draw() via a task.
            
            bool visible = self->is_effectively_visible();
            if (self->application() && visible) {
                if (!self->m_waiting_for_draw.exchange(true)) {
                    auto alive = self->m_alive_flag;
                    self->application()->post_task([self, alive]() {
                        if (!*alive) return;
                        self->invalidate();
                    });
                }
            } else if (!visible) {
                // For invisible tabs, we don't invalidate. We'll wait for a manual invalidate 
                // when they become visible (handled by TabCollection).
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

            double effective_view_h = (m_target_view_h > 0) ? m_target_view_h : height();
            double effective_view_w = (m_target_view_w > 0) ? m_target_view_w : width();

            if (effective_view_h <= 0 || effective_view_w <= 0) return;

            // PERSISTENT VISIBILITY: Always show the gutter for responsiveness, unless inspector is open
            m_show_v_scroll = !m_inspector_visible; 
            m_show_h_scroll = (m_content_width > effective_view_w + 1) && !m_inspector_visible;

            if (m_show_v_scroll) {
                m_v_track_x = x() + width() - SCROLLBAR_SIZE - 2;
                m_v_track_y = y() + 2;
                m_v_track_w = SCROLLBAR_SIZE;
                m_v_track_h = (int)effective_view_h - 4 - (m_show_h_scroll ? SCROLLBAR_SIZE : 0);

                double visible_ratio = effective_view_h / m_content_height;
                if (!std::isfinite(visible_ratio) || visible_ratio > 1.0) visible_ratio = 1.0;
                m_v_thumb_h = std::max(20, (int)(m_v_track_h * visible_ratio));
                
                double scrollable_height = m_content_height - effective_view_h;
                double scroll_ratio = (scrollable_height > 0) ? (m_scroll_y / scrollable_height) : 0;
                scroll_ratio = std::max(0.0, std::min(1.0, scroll_ratio));
                
                m_v_thumb_y = m_v_track_y + (int)(scroll_ratio * (m_v_track_h - m_v_thumb_h));
            }

            if (m_show_h_scroll) {
                m_h_track_x = x() + 2;
                m_h_track_y = y() + height() - SCROLLBAR_SIZE - 2;
                m_h_track_w = width() - 4 - (m_show_v_scroll ? SCROLLBAR_SIZE : 0);
                m_h_track_h = SCROLLBAR_SIZE;
 
                double visible_ratio = effective_view_w / m_content_width;
                if (!std::isfinite(visible_ratio) || visible_ratio > 1.0) visible_ratio = 1.0;
                m_h_thumb_w = std::max(20, (int)(m_h_track_w * visible_ratio));
                
                double scrollable_width = m_content_width - effective_view_w;
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
                        webkit_web_view_load_uri(pair->first->m_web_view, pair->second.c_str());
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

            {
                std::lock_guard<std::mutex> lock(m_surface_mutex);
                m_waiting_for_draw.store(false);
                
                if (m_has_front_buffer.load() && m_cairo_surface_front)
                {
                    cairo_t *cr = (cairo_t *)ctx.getNativeContext();
                    if (cr && cairo_status(cr) == CAIRO_STATUS_SUCCESS) {
                        cairo_save(cr);
                        cairo_rectangle(cr, x(), y(), width(), height());
                        cairo_clip(cr);
                        
                        int sw = cairo_image_surface_get_width(m_cairo_surface_front);
                        int sh = cairo_image_surface_get_height(m_cairo_surface_front);
                        if (sw > 0 && sh > 0) {
                            double scale_x = (double)width() / sw;
                            double scale_y = (double)height() / sh;
                            cairo_translate(cr, x(), y());
                            cairo_scale(cr, scale_x, scale_y);
                            cairo_set_source_surface(cr, m_cairo_surface_front, 0, 0);
                            cairo_paint(cr);
                        }
                        cairo_restore(cr);
                    }
                }
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
            m_is_visible_cached.store(is_effectively_visible());

            if (m_initialized && m_backend && width() > 0 && height() > 0 && s_worker_context)
            {
                if (!m_is_fullscreen && width() == m_last_dispatched_width && height() == m_last_dispatched_height) return;
                
                m_last_dispatched_width = width();
                m_last_dispatched_height = height();

                if (m_is_fullscreen && m_waiting_for_native_frame) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_fullscreen_time).count();
                    if (elapsed_ms < 2000) return;
                }

                struct ResizeData {
                    WebView* self;
                    struct wpe_view_backend* backend;
                    int w;
                    int h;
                };

                int target_w = (m_is_fullscreen && application()) ? application()->width() : width();
                int target_h = (m_is_fullscreen && application()) ? application()->height() : height();

                auto* rd = new ResizeData{this, m_backend, target_w, target_h};
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* d = static_cast<ResizeData*>(data);
                    if (d->backend) wpe_view_backend_dispatch_set_size(d->backend, d->w, d->h);
                    if (d->self->m_pending_fullscreen_ack) {
                        wpe_view_backend_platform_set_fullscreen(d->backend, d->self->m_is_fullscreen);
                        d->self->m_pending_fullscreen_ack = false;
                        d->self->m_waiting_for_native_frame = true;
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


        bool WebView::can_perform(horizon::ClipboardAction action) const {
            if (action == horizon::ClipboardAction::Paste) return true;
            // For Copy/Cut, we'd ideally check webkit_web_view_can_execute_editing_command
            // but for simplicity and since we have text selection working, we'll allow it.
            return true; 
        }

        void WebView::perform(horizon::ClipboardAction action) {
            if (!m_web_view) return;

            if (action == horizon::ClipboardAction::Copy || action == horizon::ClipboardAction::Cut) {
                const char* cmd = (action == horizon::ClipboardAction::Copy) ? "copy" : "cut";
                
                // 1. Execute everything on the WebKit worker thread
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    auto* self = static_cast<WebView*>(data);
                    
                    // a) Execute native command
                    webkit_web_view_execute_editing_command(self->m_web_view, "copy");

                    // b) Evaluate JS to extract text for Horizon
                    const char* js = "window.getSelection().toString()";
                    webkit_web_view_evaluate_javascript(self->m_web_view, js, -1, NULL, NULL, NULL, 
                        +[](GObject*, GAsyncResult* res, gpointer user_data) {
                            WebView* self = static_cast<WebView*>(user_data);
                            GError* error = NULL;
                            JSCValue* value = webkit_web_view_evaluate_javascript_finish(self->m_web_view, res, &error);
                            if (value) {
                                char* text = jsc_value_to_string(value);
                                if (text) {
                                    self->m_clipboard_content = text;
                                    if (self->application()) {
                                        self->application()->set_clipboard_owner(self);
                                    }
                                    g_free(text);
                                }
                                g_object_unref(value);
                            }
                            if (error) {
                                g_error_free(error);
                            }
                        }, self);

                    return FALSE;
                }, this);

            } else if (action == horizon::ClipboardAction::Paste) {
                if (application()) {
                    application()->request_clipboard_data(this, "text/plain");
                }
            }
        }

        void WebView::provide_clipboard_data(const std::string& mime, horizon::DataSink& sink) {
            if (mime == "text/plain" || mime == "text/plain;charset=utf-8") {
                if (!m_clipboard_content.empty()) {
                    std::vector<uint8_t> data(m_clipboard_content.begin(), m_clipboard_content.end());
                    sink.write(data);
                    sink.done();
                }
            } else {
                sink.error();
            }
        }

        std::vector<std::string> WebView::provided_mime_types() const {
            return {"text/plain", "text/plain;charset=utf-8"};
        }

        std::vector<std::string> WebView::accepted_mime_types() const {
            return {"text/plain", "text/plain;charset=utf-8"};
        }

        void WebView::on_clipboard_data_received(const std::string& mime, const std::vector<uint8_t>& data) {
            if (mime == "text/plain" || mime == "text/plain;charset=utf-8") {
                std::string text((const char*)data.data(), data.size());
                
                // Escape text for JS
                std::string escaped;
                for (char c : text) {
                    if (c == '\'') escaped += "\\'";
                    else if (c == '\\') escaped += "\\\\";
                    else if (c == '\n') escaped += "\\n";
                    else if (c == '\r') escaped += "\\r";
                    else escaped += c;
                }

                std::string js = "document.execCommand('insertText', false, '" + escaped + "')";
                
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* d) -> gboolean {
                    auto* p = static_cast<std::pair<WebView*, std::string>*>(d);
                    webkit_web_view_evaluate_javascript(p->first->m_web_view, p->second.c_str(), -1, NULL, NULL, NULL, NULL, NULL);
                    delete p;
                    return FALSE;
                }, new std::pair<WebView*, std::string>(this, js));
            }
        }
    } // namespace web
} // namespace horizon
