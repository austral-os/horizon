#include <horizon/image/ImageWidget.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/WaylandWindow.hpp>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace horizon {
namespace image {

ImageWidget::ImageWidget() {
}

ImageWidget::~ImageWidget() {
}

void ImageWidget::set_path(const std::string& path) {
    if (m_path == path) return;
    m_path = path;
    load_driver();
    update_size();
    invalidate();
}

void ImageWidget::set_zoom(float zoom) {
    m_zoom = std::max(0.01f, zoom);
    update_size();
    invalidate();
}

void ImageWidget::set_rotation(float rotation) {
    m_rotation = rotation;
    // Normalize rotation to [0, 360)
    while (m_rotation < 0) m_rotation += 360.0f;
    while (m_rotation >= 360.0f) m_rotation -= 360.0f;
    update_size();
    invalidate();
}

void ImageWidget::zoom_in() {
    set_zoom(m_zoom * 1.2f);
}

void ImageWidget::zoom_out() {
    set_zoom(m_zoom / 1.2f);
}

void ImageWidget::zoom_fit(int container_w, int container_h) {
    if (!m_driver || container_w <= 0 || container_h <= 0) return;

    int iw = image_width();
    int ih = image_height();
    
    // Calculate size after rotation (assuming 90 deg increments for simplicity)
    int rot_w = iw;
    int rot_h = ih;
    int rot = (int)std::round(m_rotation) % 360;
    if (rot == 90 || rot == 270) {
        std::swap(rot_w, rot_h);
    }

    float zoom_x = (float)container_w / rot_w;
    float zoom_y = (float)container_h / rot_h;
    set_zoom(std::min(zoom_x, zoom_y) * 0.95f); // 5% margin
}

void ImageWidget::original_size() {
    set_zoom(1.0f);
}

void ImageWidget::rotate_cw() {
    set_rotation(m_rotation + 90.0f);
}

void ImageWidget::rotate_ccw() {
    set_rotation(m_rotation - 90.0f);
}

int ImageWidget::image_width() const {
    return m_driver ? m_driver->width() : 0;
}

int ImageWidget::image_height() const {
    return m_driver ? m_driver->height() : 0;
}

void ImageWidget::set_application_recursive(WaylandWindow* app) {
    Widget::set_application_recursive(app);
    if (app && !m_driver && !m_path.empty()) {
        load_driver();
        update_size();
        invalidate();
    }
}

void ImageWidget::load_driver() {
    if (m_path.empty()) return;
    auto app = application();
    if (!app) return;
    
    // Use a dummy graphics context if necessary or get it from application
    // Actually, GraphicsContext is needed to create the driver (factory pattern)
    // We can use the window's context
    auto* win = dynamic_cast<WaylandWindow*>(app);
    if (!win) return;
    
    auto& ctx = win->get_graphics_context();
    m_driver = ctx.createImageDriver(m_path);
}

void ImageWidget::update_size() {
    if (!m_driver) return;

    int iw = m_driver->width();
    int ih = m_driver->height();

    int rot = (int)std::round(m_rotation) % 360;
    if (rot == 90 || rot == 270) {
        std::swap(iw, ih);
    }

    set_size((int)(iw * m_zoom), (int)(ih * m_zoom));
}

void ImageWidget::draw(GraphicsContext& ctx) {
    if (!m_driver) return;

    ctx.save();

    // Center the image in the widget area if it's larger than the image
    // but usually fixed_size matches the zoomed image.
    int w = width();
    int h = height();

    ctx.translate(x() + w / 2.0f, y() + h / 2.0f);
    ctx.rotate(m_rotation * (M_PI / 180.0f));
    ctx.scale(m_zoom, m_zoom);
    
    int iw = m_driver->width();
    int ih = m_driver->height();

    // Draw centering the image around the current origin
    m_driver->draw(ctx, -iw / 2, -ih / 2, iw, ih);

    ctx.restore();
}

} // namespace image
} // namespace horizon
