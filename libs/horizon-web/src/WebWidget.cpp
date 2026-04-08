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
                                                             0, // state handled by wpe
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

            // 1. Initialize WPE loader
            wpe_loader_init("/usr/lib/x86_64-linux-gnu/libWPEBackend-fdo-1.0.so.1");
            
            // 2. Initialize the SHM backend for FDO
            wpe_fdo_initialize_shm();

            // 3. Define the client for the exportable
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

            // 4. Create the exportable and get the backend
            int initial_width = width() > 0 ? width() : 800;
            int initial_height = height() > 0 ? height() : 600;

            m_exportable = wpe_view_backend_exportable_fdo_create(&client, this, initial_width, initial_height);
            if (!m_exportable) {
                LOG_ERROR << "[WebWidget] Failed to create WPE FDO exportable";
                return;
            }

            m_backend = wpe_view_backend_exportable_fdo_get_view_backend(m_exportable);
            if (!m_backend) {
                LOG_ERROR << "[WebWidget] Failed to get WPE view backend from exportable";
                return;
            }

            // 5. Create the WebKit web view
            auto *webkit_backend = webkit_web_view_backend_new(m_backend, nullptr, nullptr);
            m_web_view = webkit_web_view_new(webkit_backend);
            
            if (!m_web_view) {
                LOG_ERROR << "[WebWidget] Failed to create WebKit web view";
                return;
            }

            // 6. Integrate GLib Loop into Horizon using a Main Thread timer
            // This avoids threading conflicts by pumping GLib events once per frame (16ms)
            // in the same thread where WebKit was initialized.
            if (application()) {
                application()->add_timer(16, []() {
                    g_main_context_iteration(NULL, FALSE);
                }, true);
            }

            LOG_INFO << "WPE WebKit (Nova) engine initialized with timer-based GLib pump";
            m_initialized = true;
        }

        void WebWidget::on_frame_exported(void *data, struct wpe_fdo_shm_exported_buffer *buffer)
        {
            auto *self = static_cast<WebWidget *>(data);
            if (!self->m_exportable) return;

            struct wl_shm_buffer *shm_buffer = wpe_fdo_shm_exported_buffer_get_shm_buffer(buffer);
            if (!shm_buffer)
            {
                wpe_view_backend_exportable_fdo_dispatch_release_shm_exported_buffer(
                    self->m_exportable, buffer);
                return;
            }

            // Get buffer info
            int width = wl_shm_buffer_get_width(shm_buffer);
            int height = wl_shm_buffer_get_height(shm_buffer);
            void *buffer_data = wl_shm_buffer_get_data(shm_buffer);
            int stride = wl_shm_buffer_get_stride(shm_buffer);

            // Update or create Cairo surface
            if (!self->m_cairo_surface ||
                cairo_image_surface_get_width(self->m_cairo_surface) != width ||
                cairo_image_surface_get_height(self->m_cairo_surface) != height)
            {

                if (self->m_cairo_surface)
                    cairo_surface_destroy(self->m_cairo_surface);
                self->m_cairo_surface =
                    cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
            }

            // Copy data (WPE SHM is usually ARGB32)
            unsigned char *dest = cairo_image_surface_get_data(self->m_cairo_surface);
            if (dest && buffer_data) {
                cairo_surface_flush(self->m_cairo_surface);
                std::memcpy(dest, buffer_data, stride * height);
                cairo_surface_mark_dirty(self->m_cairo_surface);
            }

            // Release buffer back to WPE
            wpe_view_backend_exportable_fdo_dispatch_release_shm_exported_buffer(self->m_exportable,
                                                                                 buffer);
            
            // Notify WPE that we processed the frame.
            wpe_view_backend_exportable_fdo_dispatch_frame_complete(self->m_exportable);

            // Request redraw
            self->invalidate();
        }

        void WebWidget::load_url(const std::string &url)
        {
            if (!m_initialized)
                init_wpe();
            
            if (m_web_view) {
                LOG_INFO << "[WebWidget] Loading URL: " << url;
                webkit_web_view_load_uri(m_web_view, url.c_str());
            }
        }

        void WebWidget::draw(GraphicsContext &ctx)
        {
            // Fill background
            ctx.setColor(background_color());
            ctx.fillRect(x(), y(), width(), height());

            if (m_cairo_surface)
            {
                cairo_t *cr = (cairo_t *)ctx.getNativeContext();
                cairo_set_source_surface(cr, m_cairo_surface, x(), y());
                cairo_paint(cr);
            }
            else
            {
                // Draw loading placeholder
                ctx.setColor(Color(0.95f, 0.95f, 0.95f, 1.0f));
                ctx.fillRect(x(), y(), width(), height());
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

        void WebWidget::reload()
        {
            if (m_web_view) webkit_web_view_reload(m_web_view);
        }
        void WebWidget::stop_loading()
        {
            if (m_web_view) webkit_web_view_stop_loading(m_web_view);
        }
        void WebWidget::go_back()
        {
            if (m_web_view) webkit_web_view_go_back(m_web_view);
        }
        void WebWidget::go_forward()
        {
            if (m_web_view) webkit_web_view_go_forward(m_web_view);
        }

        bool WebWidget::can_go_back() const
        {
            return m_web_view && webkit_web_view_can_go_back(m_web_view);
        }
        bool WebWidget::can_go_forward() const
        {
            return m_web_view && webkit_web_view_can_go_forward(m_web_view);
        }

    } // namespace web
} // namespace horizon
