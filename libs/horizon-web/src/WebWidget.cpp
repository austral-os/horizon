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

        WebWidget::WebWidget()
        {
            set_focusable(true);
            set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));

            // Mouse Press
            when_mouse_press.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    if (!m_backend)
                        return;
                    struct wpe_input_pointer_event event = {wpe_input_pointer_event_type_button,
                                                             0, 
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             ctx.button,
                                                             1,
                                                             0};
                    wpe_view_backend_dispatch_pointer_event(m_backend, &event);
                });

            // Mouse Release
            when_mouse_release.connect(
                [this](MouseButtonEventContext &ctx)
                {
                    if (!m_backend)
                        return;
                    struct wpe_input_pointer_event event = {wpe_input_pointer_event_type_button,
                                                             0,
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             ctx.button,
                                                             0,
                                                             0};
                    wpe_view_backend_dispatch_pointer_event(m_backend, &event);
                });

            // Mouse Move
            when_mouse_move.connect(
                [this](MouseMoveEventContext &ctx)
                {
                    if (!m_backend)
                        return;
                    struct wpe_input_pointer_event event = {wpe_input_pointer_event_type_motion,
                                                             0,
                                                             (int)(ctx.x - x()),
                                                             (int)(ctx.y - y()),
                                                             0,
                                                             0,
                                                             0};
                    wpe_view_backend_dispatch_pointer_event(m_backend, &event);
                });

            // Mouse Wheel
            when_mouse_wheel.connect(
                [this](MouseWheelEventContext &ctx)
                {
                    if (!m_backend)
                        return;
                    // Forward vertical scroll
                    struct wpe_input_axis_event event = {
                        wpe_input_axis_event_type_mask_2d,
                        (int)ctx.dy * -20, // Negative for natural scroll matching
                        (int)(ctx.x - x()),
                        (int)(ctx.y - y()),
                        0};
                    wpe_view_backend_dispatch_axis_event(m_backend, &event);
                });

            // Keyboard
            when_key_press.connect(
                [this](KeyEventContext &ctx)
                {
                    if (!m_backend)
                        return;
                    struct wpe_input_keyboard_event event = {0, ctx.key, ctx.keysym, true,
                                                             ctx.modifiers};
                    wpe_view_backend_dispatch_keyboard_event(m_backend, &event);
                });

            when_key_release.connect(
                [this](KeyEventContext &ctx)
                {
                    if (!m_backend)
                        return;
                    struct wpe_input_keyboard_event event = {0, ctx.key, ctx.keysym, false,
                                                             ctx.modifiers};
                    wpe_view_backend_dispatch_keyboard_event(m_backend, &event);
                });
        }

        WebWidget::~WebWidget()
        {
            if (m_web_view)
            {
                g_object_unref(m_web_view);
            }
            if (m_backend)
            {
                wpe_view_backend_destroy(m_backend);
            }
            if (m_cairo_surface)
            {
                cairo_surface_destroy(m_cairo_surface);
            }
        }

        void WebWidget::init_wpe()
        {
            if (m_initialized)
                return;

            wpe_loader_init("/usr/lib/x86_64-linux-gnu/libWPEBackend-fdo-1.0.so.1");
            wpe_fdo_initialize_shm();

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

            int initial_width = width() > 0 ? width() : 800;
            int initial_height = height() > 0 ? height() : 600;

            m_exportable = wpe_view_backend_exportable_fdo_create(&client, this, initial_width, initial_height);
            m_backend = wpe_view_backend_exportable_fdo_get_view_backend(m_exportable);

            auto *webkit_backend = webkit_web_view_backend_new(m_backend, nullptr, nullptr);
            m_web_view = webkit_web_view_new(webkit_backend);

            // Connect Signals for Refinement
            g_signal_connect(m_web_view, "notify::title", G_CALLBACK(on_title_notify), this);
            g_signal_connect(m_web_view, "notify::uri", G_CALLBACK(on_uri_notify), this);
            g_signal_connect(m_web_view, "load-changed", G_CALLBACK(on_load_changed), this);
            g_signal_connect(m_web_view, "notify::estimated-load-progress", G_CALLBACK(on_progress_notify), this);
            g_signal_connect(m_web_view, "mouse-target-changed", G_CALLBACK(on_mouse_target_changed), this);

            if (application()) {
                application()->add_timer(16, []() {
                    g_main_context_iteration(NULL, FALSE);
                }, true);
            }

            LOG_INFO << "WPE WebKit (Nova) engine initialized with refinement signals connected";
            m_initialized = true;
        }

        void WebWidget::on_title_notify(WebKitWebView*, GParamSpec*, WebWidget* self) {
            std::string title = self->get_title();
            EventContext ctx;
            self->when_title_changed.run(title);
        }

        void WebWidget::on_uri_notify(WebKitWebView*, GParamSpec*, WebWidget* self) {
            std::string url = self->get_url();
            self->when_url_changed.run(url);
        }

        void WebWidget::on_load_changed(WebKitWebView*, int load_event, WebWidget* self) {
            bool loading = (load_event != 3); // 3 == WEBKIT_LOAD_FINISHED
            self->when_loading_changed.run(loading);
        }

        void WebWidget::on_progress_notify(WebKitWebView* web_view, GParamSpec*, WebWidget* self) {
            double progress = webkit_web_view_get_estimated_load_progress(web_view);
            self->when_progress_changed.run(progress);
        }

        void WebWidget::on_mouse_target_changed(WebKitWebView*, void*, uint32_t, WebWidget* self) {
            // This happens when hovering over links.
            // Simplified: change cursor to pointer if it's a link (handled by WebKit internally via cursor signals)
            // But WPE FDO doesn't always handle it automatically, so WebKit handles it via g_signal_connect "cursor-changed"
            // For now, let's just use the default cursor or let WebKit handle it if possible.
        }

        std::string WebWidget::get_title() const {
             if (!m_web_view) return "";
             const char* title = webkit_web_view_get_title(m_web_view);
             return title ? title : "";
        }

        std::string WebWidget::get_url() const {
             if (!m_web_view) return "";
             const char* uri = webkit_web_view_get_uri(m_web_view);
             return uri ? uri : "";
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

            wpe_view_backend_exportable_fdo_dispatch_release_shm_exported_buffer(self->m_exportable, buffer);
            wpe_view_backend_exportable_fdo_dispatch_frame_complete(self->m_exportable);
            self->invalidate();
        }

        void WebWidget::load_url(const std::string &url)
        {
            if (!m_initialized) init_wpe();
            if (m_web_view) webkit_web_view_load_uri(m_web_view, url.c_str());
        }

        void WebWidget::draw(GraphicsContext &ctx)
        {
            ctx.setColor(background_color());
            ctx.fillRect(x(), y(), width(), height());

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
            if (m_initialized && m_backend && width() > 0 && height() > 0)
            {
                wpe_view_backend_dispatch_set_size(m_backend, width(), height());
            }
            else if (!m_initialized && width() > 0 && height() > 0)
            {
                init_wpe();
            }
        }

        void WebWidget::reload() { if (m_web_view) webkit_web_view_reload(m_web_view); }
        void WebWidget::stop_loading() { if (m_web_view) webkit_web_view_stop_loading(m_web_view); }
        void WebWidget::go_back() { if (m_web_view) webkit_web_view_go_back(m_web_view); }
        void WebWidget::go_forward() { if (m_web_view) webkit_web_view_go_forward(m_web_view); }

        bool WebWidget::can_go_back() const { return m_web_view && webkit_web_view_can_go_back(m_web_view); }
        bool WebWidget::can_go_forward() const { return m_web_view && webkit_web_view_can_go_forward(m_web_view); }

    } // namespace web
} // namespace horizon
