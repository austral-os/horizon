#include "CroppableImageWidget.hpp"
#include <horizon/WaylandWindow.hpp>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <horizon/Logger.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace horizon {
namespace image {

CroppableImageWidget::CroppableImageWidget() {
    when_mouse_press.connect([this](MouseButtonEventContext& ctx) {
        if (!m_is_cropping) return;
        
        float ix, iy;
        screen_to_image(ctx.x, ctx.y, ix, iy);
        
        // Find if clicking a handle
        float threshold = 15.0f / zoom();
        
        auto check_handle = [&](float hx, float hy) {
            return std::abs(ix - hx) < threshold && std::abs(iy - hy) < threshold;
        };
        
        if (check_handle(m_crop_x, m_crop_y)) m_dragging_handle = 0;
        else if (check_handle(m_crop_x + m_crop_w, m_crop_y)) m_dragging_handle = 1;
        else if (check_handle(m_crop_x + m_crop_w, m_crop_y + m_crop_h)) m_dragging_handle = 2;
        else if (check_handle(m_crop_x, m_crop_y + m_crop_h)) m_dragging_handle = 3;
        else if (check_handle(m_crop_x + m_crop_w / 2, m_crop_y)) m_dragging_handle = 5; // top-center
        else if (check_handle(m_crop_x + m_crop_w, m_crop_y + m_crop_h / 2)) m_dragging_handle = 6; // right-center
        else if (check_handle(m_crop_x + m_crop_w / 2, m_crop_y + m_crop_h)) m_dragging_handle = 7; // bottom-center
        else if (check_handle(m_crop_x, m_crop_y + m_crop_h / 2)) m_dragging_handle = 8; // left-center
        else if (ix >= m_crop_x && ix <= m_crop_x + m_crop_w && iy >= m_crop_y && iy <= m_crop_y + m_crop_h) m_dragging_handle = 4;
        else {
            // New crop rect
            m_dragging_handle = 2; // drag bottom right
            m_crop_x = ix;
            m_crop_y = iy;
            m_crop_w = 0;
            m_crop_h = 0;
        }
        
        m_drag_start_x = ix;
        m_drag_start_y = iy;
        m_crop_start_x = m_crop_x;
        m_crop_start_y = m_crop_y;
        m_crop_start_w = m_crop_w;
        m_crop_start_h = m_crop_h;
        
        invalidate();
    });
    
    when_mouse_drag.connect([this](MouseMoveEventContext& ctx) {
        if (!m_is_cropping || m_dragging_handle == -1) return;
        
        float ix, iy;
        screen_to_image(ctx.x, ctx.y, ix, iy);
        
        float dx = ix - m_drag_start_x;
        float dy = iy - m_drag_start_y;
        
        if (m_dragging_handle == 0) { // top-left
            m_crop_x = m_crop_start_x + dx;
            m_crop_y = m_crop_start_y + dy;
            m_crop_w = m_crop_start_w - dx;
            m_crop_h = m_crop_start_h - dy;
        } else if (m_dragging_handle == 1) { // top-right
            m_crop_y = m_crop_start_y + dy;
            m_crop_w = m_crop_start_w + dx;
            m_crop_h = m_crop_start_h - dy;
        } else if (m_dragging_handle == 2) { // bottom-right
            m_crop_w = m_crop_start_w + dx;
            m_crop_h = m_crop_start_h + dy;
        } else if (m_dragging_handle == 3) { // bottom-left
            m_crop_x = m_crop_start_x + dx;
            m_crop_w = m_crop_start_w - dx;
            m_crop_h = m_crop_start_h + dy;
        } else if (m_dragging_handle == 5) { // top-center
            m_crop_y = m_crop_start_y + dy;
            m_crop_h = m_crop_start_h - dy;
        } else if (m_dragging_handle == 6) { // right-center
            m_crop_w = m_crop_start_w + dx;
        } else if (m_dragging_handle == 7) { // bottom-center
            m_crop_h = m_crop_start_h + dy;
        } else if (m_dragging_handle == 8) { // left-center
            m_crop_x = m_crop_start_x + dx;
            m_crop_w = m_crop_start_w - dx;
        } else if (m_dragging_handle == 4) { // center
            m_crop_x = m_crop_start_x + dx;
            m_crop_y = m_crop_start_y + dy;
        }
        
        invalidate();
    });
    
    when_mouse_release.connect([this](MouseButtonEventContext& ctx) {
        if (!m_is_cropping) return;
        
        // Normalize rect
        if (m_crop_w < 0) {
            m_crop_x += m_crop_w;
            m_crop_w = -m_crop_w;
        }
        if (m_crop_h < 0) {
            m_crop_y += m_crop_h;
            m_crop_h = -m_crop_h;
        }
        
        // Clamp to image bounds
        if (m_crop_x < 0) m_crop_x = 0;
        if (m_crop_y < 0) m_crop_y = 0;
        if (m_crop_x + m_crop_w > image_width()) m_crop_w = image_width() - m_crop_x;
        if (m_crop_y + m_crop_h > image_height()) m_crop_h = image_height() - m_crop_y;
        
        m_dragging_handle = -1;
        invalidate();
    });
    
    when_key_press.connect([this](KeyEventContext& ctx) {
        if (!m_is_cropping) return;
        if (ctx.keysym == XKB_KEY_Return || ctx.keysym == XKB_KEY_KP_Enter) {
            apply_crop();
        } else if (ctx.keysym == XKB_KEY_Escape) {
            cancel_crop();
        }
    });

    when_undo.connect([this](EventContext&) {
        undo_crop();
    });

    when_redo.connect([this](EventContext&) {
        redo_crop();
    });
}

CroppableImageWidget::~CroppableImageWidget() {
    if (!m_current_temp_path.empty() && std::filesystem::exists(m_current_temp_path)) {
        std::filesystem::remove(m_current_temp_path);
    }
}

void CroppableImageWidget::toggle_crop_mode() {
    if (m_original_path.empty() && !path().empty()) {
        m_original_path = path();
    }
    m_is_cropping = !m_is_cropping;
    if (m_is_cropping) {
        reset_crop_rect();
    }
    invalidate();
    EventContext ev;
    when_crop_mode_changed.run(ev);
}

void CroppableImageWidget::reset_crop_rect() {
    m_crop_x = image_width() * 0.1f;
    m_crop_y = image_height() * 0.1f;
    m_crop_w = image_width() * 0.8f;
    m_crop_h = image_height() * 0.8f;
}

void CroppableImageWidget::apply_crop() {
    if (!m_is_cropping || m_crop_w <= 0 || m_crop_h <= 0) return;
    
    if (m_original_path.empty()) m_original_path = path();
    
    // Generate temp file
    std::string temp_path = "/tmp/horizon_crop_" + std::to_string(std::rand()) + ".png";
    
    std::string current_p = m_current_temp_path.empty() ? m_original_path : m_current_temp_path;
    
    // Using ImageMagick
    std::string cmd = "convert \"" + current_p + "\" -crop " + 
        std::to_string((int)m_crop_w) + "x" + std::to_string((int)m_crop_h) + "+" + 
        std::to_string((int)m_crop_x) + "+" + std::to_string((int)m_crop_y) + 
        " +repage \"" + temp_path + "\"";
        
    EventContext ev;
    when_operation_started.run(ev);
    
    auto app = application();
    std::thread([this, app, current_p, temp_path, cmd]() {
        int ret = std::system(cmd.c_str());
        if (app) {
            app->post_task([this, ret, current_p, temp_path]() {
                if (ret == 0) {
                    m_history.push_back(current_p);
                    
                    // Clear redo history when a new action is performed
                    for (const auto& p : m_redo_history) {
                        if (p != m_original_path && std::filesystem::exists(p)) {
                            std::filesystem::remove(p);
                        }
                    }
                    m_redo_history.clear();
                    
                    m_current_temp_path = temp_path;
                    
                    m_is_cropping = false;
                    
                    // Reload image from temp path
                    set_path(""); // force reload
                    set_path(m_current_temp_path);
                    
                    EventContext ev_crop;
                    this->when_crop_mode_changed.run(ev_crop);
                } else {
                    LOG_ERROR << "Failed to apply crop via ImageMagick";
                }
                
                EventContext ev_end;
                this->when_operation_finished.run(ev_end);
            });
        }
    }).detach();
}

void CroppableImageWidget::undo_crop() {
    if (m_history.empty()) return;
    
    std::string current_p = m_current_temp_path.empty() ? m_original_path : m_current_temp_path;
    m_redo_history.push_back(current_p);
    
    m_current_temp_path = m_history.back();
    m_history.pop_back();
    
    if (m_current_temp_path == m_original_path) {
        m_current_temp_path.clear();
    }
    
    std::string path_to_load = m_current_temp_path.empty() ? m_original_path : m_current_temp_path;
    
    set_path(""); // force reload
    set_path(path_to_load);
    
    if (m_is_cropping) {
        reset_crop_rect();
        invalidate();
    }
}

void CroppableImageWidget::redo_crop() {
    if (m_redo_history.empty()) return;

    std::string current_p = m_current_temp_path.empty() ? m_original_path : m_current_temp_path;
    m_history.push_back(current_p);

    m_current_temp_path = m_redo_history.back();
    m_redo_history.pop_back();

    if (m_current_temp_path == m_original_path) {
        m_current_temp_path.clear();
    }

    std::string path_to_load = m_current_temp_path.empty() ? m_original_path : m_current_temp_path;
    
    set_path(""); // force reload
    set_path(path_to_load);
    
    if (m_is_cropping) {
        reset_crop_rect();
        invalidate();
    }
}

void CroppableImageWidget::save_to_path(const std::string& target_path) {
    if (m_original_path.empty() && m_current_temp_path.empty()) {
        if (!path().empty()) {
            m_original_path = path();
        } else {
            return;
        }
    }
    
    EventContext ev;
    when_operation_started.run(ev);
    
    auto app = application();
    std::string src = m_current_temp_path.empty() ? m_original_path : m_current_temp_path;
    std::string dst = target_path;
    
    std::thread([this, app, src, dst]() {
        std::error_code ec;
        if (src != dst) {
            std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
        }
        
        if (app) {
            app->post_task([this, ec, dst]() {
                if (!ec) {
                    if (!m_current_temp_path.empty() && m_current_temp_path != dst) {
                        std::filesystem::remove(m_current_temp_path);
                        m_current_temp_path.clear();
                    }
                    m_original_path = dst;
                    m_history.clear(); // Clear history after save
                    for (const auto& p : m_redo_history) {
                        if (p != m_original_path && std::filesystem::exists(p)) {
                            std::filesystem::remove(p);
                        }
                    }
                    m_redo_history.clear(); // Clear redo history after save
                    set_path(m_original_path);
                } else {
                    LOG_ERROR << "Failed to save image to path: " << ec.message();
                }
                
                EventContext ev_end;
                this->when_operation_finished.run(ev_end);
            });
        }
    }).detach();
}

void CroppableImageWidget::save_image() {
    if (m_original_path.empty()) return;
    save_to_path(m_original_path);
}

void CroppableImageWidget::cancel_crop() {
    m_is_cropping = false;
    invalidate();
    EventContext ev;
    when_crop_mode_changed.run(ev);
}

void CroppableImageWidget::screen_to_image(float sx, float sy, float& ix, float& iy) {
    float cx = x() + width() / 2.0f;
    float cy = y() + height() / 2.0f;
    
    float dx = sx - cx;
    float dy = sy - cy;
    
    // Unscale
    dx /= zoom();
    dy /= zoom();
    
    // Unrotate (negative rotation)
    float rad = -rotation() * (M_PI / 180.0f);
    float rx = dx * std::cos(rad) - dy * std::sin(rad);
    float ry = dx * std::sin(rad) + dy * std::cos(rad);
    
    ix = rx + image_width() / 2.0f;
    iy = ry + image_height() / 2.0f;
}

void CroppableImageWidget::draw(GraphicsContext& ctx) {
    ImageWidget::draw(ctx);
    
    if (m_is_cropping) {
        ctx.save();
        
        ctx.translate(x() + width() / 2.0f, y() + height() / 2.0f);
        ctx.rotate(rotation() * (M_PI / 180.0f));
        ctx.scale(zoom(), zoom());
        
        float cx = m_crop_x - image_width() / 2.0f;
        float cy = m_crop_y - image_height() / 2.0f;
        
        // Draw dark overlay outside crop rect
        ctx.setColor(0.0f, 0.0f, 0.0f, 0.5f);
        float iw2 = image_width() / 2.0f;
        float ih2 = image_height() / 2.0f;
        
        // Top
        ctx.fillRect(-iw2, -ih2, image_width(), m_crop_y);
        // Bottom
        ctx.fillRect(-iw2, cy + m_crop_h, image_width(), image_height() - (m_crop_y + m_crop_h));
        // Left
        ctx.fillRect(-iw2, cy, m_crop_x, m_crop_h);
        // Right
        ctx.fillRect(cx + m_crop_w, cy, image_width() - (m_crop_x + m_crop_w), m_crop_h);
        
        // Draw guides when dragging to clarify the crop area
        if (m_dragging_handle != -1 && m_dragging_handle != 4) {
            ctx.setColor(1.0f, 1.0f, 1.0f, 0.4f); // Semi-transparent white
            float guide_w = 1.0f / zoom();
            // Horizontal guides extending across the full image
            ctx.fillRect(-iw2, cy, image_width(), guide_w);
            ctx.fillRect(-iw2, cy + m_crop_h, image_width(), guide_w);
            // Vertical guides extending across the full image
            ctx.fillRect(cx, -ih2, guide_w, image_height());
            ctx.fillRect(cx + m_crop_w, -ih2, guide_w, image_height());
        }
        
        // Draw crop rect border
        ctx.setColor(1.0f, 1.0f, 1.0f, 1.0f);
        float line_w = 2.0f / zoom();
        ctx.drawRect(cx, cy, m_crop_w, m_crop_h, 0, line_w);
        
        // Draw handles
        float hw = 10.0f / zoom();
        ctx.setColor(1.0f, 1.0f, 1.0f, 1.0f);
        // Corners
        ctx.fillRect(cx - hw/2, cy - hw/2, hw, hw);
        ctx.fillRect(cx + m_crop_w - hw/2, cy - hw/2, hw, hw);
        ctx.fillRect(cx + m_crop_w - hw/2, cy + m_crop_h - hw/2, hw, hw);
        ctx.fillRect(cx - hw/2, cy + m_crop_h - hw/2, hw, hw);
        
        // Edges
        ctx.fillRect(cx + m_crop_w/2 - hw/2, cy - hw/2, hw, hw); // top-center
        ctx.fillRect(cx + m_crop_w - hw/2, cy + m_crop_h/2 - hw/2, hw, hw); // right-center
        ctx.fillRect(cx + m_crop_w/2 - hw/2, cy + m_crop_h - hw/2, hw, hw); // bottom-center
        ctx.fillRect(cx - hw/2, cy + m_crop_h/2 - hw/2, hw, hw); // left-center
        
        ctx.restore();
    }
}

} // namespace image
} // namespace horizon
