#include "horizon/web/WebWidget.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/Logger.hpp"
#include "horizon/WaylandWindow.hpp"
#include <cstring>
#include <iostream>
#include <wayland-server-core.h>
#include <wpe/fdo.h>
#include <wpe/unstable/fdo-shm.h>
#include <wpe/webkit.h>
#include <glib.h>

namespace horizon
{
    namespace web
    {
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
            set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));

            // Initialization is now managed by a shared static method called from the worker thread

            // Mouse Press
            when_mouse_press.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_button,
                                                             0, 
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             ctx.button,
                                                             1,
                                                             0};
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
                    auto* event = new wpe_input_pointer_event{wpe_input_pointer_event_type_button,
                                                             0,
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             ctx.button,
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

            // Mouse Wheel
            when_mouse_wheel.connect(
                [this](MouseWheelEventContext &ctx)
                {
                    auto* event = new wpe_input_axis_event{
                        wpe_input_axis_event_type_mask_2d,
                        0u,
                        (int)(ctx.x - x()),
                        (int)(ctx.y - y()),
                        0,
                        (int)(ctx.dy * -20)};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebWidget*, wpe_input_axis_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_axis_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebWidget*, wpe_input_axis_event*>(this, event));
                });

            // Keyboard
            when_key_press.connect(
                [this](KeyEventContext &ctx)
                {
                    auto* event = new wpe_input_keyboard_event{0, ctx.key, ctx.keysym, true,
                                                             ctx.modifiers};
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
                    auto* event = new wpe_input_keyboard_event{0, ctx.key, ctx.keysym, false,
                                                             ctx.modifiers};
                    g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                        auto* d = static_cast<std::pair<WebWidget*, wpe_input_keyboard_event*>*>(data);
                        if (d->first->m_backend) wpe_view_backend_dispatch_keyboard_event(d->first->m_backend, d->second);
                        delete d->second;
                        delete d;
                        return FALSE;
                    }, new std::pair<WebWidget*, wpe_input_keyboard_event*>(this, event));
                });
        }

        WebWidget::~WebWidget()
        {
            if (m_web_view)
            {
                // We must unref webview in the worker thread context!
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    g_object_unref(data);
                    return FALSE;
                }, m_web_view);
                m_web_view = nullptr;
            }
            
            if (m_exportable) {
                g_main_context_invoke(s_worker_context, (GSourceFunc)+[](void* data) -> gboolean {
                    wpe_view_backend_exportable_fdo_destroy((wpe_view_backend_exportable_fdo*)data);
                    return FALSE;
                }, m_exportable);
                m_exportable = nullptr;
            }

            std::lock_guard<std::mutex> lock(m_surface_mutex);
            if (m_cairo_surface)
            {
                cairo_surface_destroy(m_cairo_surface);
            }

            std::lock_guard<std::mutex> ilock(s_instance_mutex);
            s_instance_count--;
            if (s_instance_count == 0) {
                // Potential cleanup of s_worker_loop if desired
            }
        }

        void WebWidget::init_wpe()
        {
            if (m_initialized)
                return;

            ensure_worker_thread();
            
            {
                std::lock_guard<std::mutex> ilock(s_instance_mutex);
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
                self->m_web_view = webkit_web_view_new(webkit_backend);
                LOG_INFO << "WebView created for WebWidget: " << self;

                g_signal_connect(self->m_web_view, "notify::title", G_CALLBACK(on_title_notify), self);
                g_signal_connect(self->m_web_view, "notify::uri", G_CALLBACK(on_uri_notify), self);
                g_signal_connect(self->m_web_view, "load-changed", G_CALLBACK(on_load_changed), self);
                g_signal_connect(self->m_web_view, "notify::estimated-load-progress", G_CALLBACK(on_progress_notify), self);
                g_signal_connect(self->m_web_view, "mouse-target-changed", G_CALLBACK(on_mouse_target_changed), self);
                
                return FALSE;
            }, this);

            m_initialized = true;
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

            wpe_fdo_initialize_shm();

            LOG_INFO << "Shared WPE WebKit worker thread started";
            
            g_main_loop_run(s_worker_loop);

            g_main_context_pop_thread_default(s_worker_context);
            g_main_loop_unref(s_worker_loop);
            g_main_context_unref(s_worker_context);
            s_worker_loop = nullptr;
            s_worker_context = nullptr;
        }

        void WebWidget::on_title_notify(WebKitWebView* web_view, GParamSpec*, WebWidget* self) {
            const char* title_str = webkit_web_view_get_title(web_view);
            std::string title = title_str ? title_str : "";
            
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
            LOG_INFO << "Load changed for WebWidget: " << self << " Event: " << load_event;
            bool loading = (load_event != 3); // 3 == WEBKIT_LOAD_FINISHED
            if (self->application()) {
                self->application()->post_task([self, loading]() {
                    bool l = loading;
                    self->when_loading_changed.run(l);
                });
            }
        }

        void WebWidget::on_progress_notify(WebKitWebView* web_view, GParamSpec*, WebWidget* self) {
            double progress = webkit_web_view_get_estimated_load_progress(web_view);
            if (self->application()) {
                self->application()->post_task([self, progress]() {
                    double p = progress;
                    self->when_progress_changed.run(p);
                });
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
                    self->invalidate();
                });
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
            ctx.setColor(background_color());
            ctx.fillRect(x(), y(), width(), height());

            std::lock_guard<std::mutex> lock(m_surface_mutex);
            if (m_cairo_surface)
            {
                cairo_t *cr = (cairo_t *)ctx.getNativeContext();
                cairo_set_source_surface(cr, m_cairo_surface, x(), y());
                cairo_paint(cr);
            }
        }

        void WebWidget::calculate_layout()
        {
            Widget::calculate_layout();
            if (m_initialized && m_backend && width() > 0 && height() > 0 && s_worker_context)
            {
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
