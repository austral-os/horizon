#include <algorithm>
#include <cmath>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/CoverFlow.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Matrix.hpp>

namespace horizon
{

    CoverFlowBase::CoverFlowBase() : Widget()
    {
        when_mouse_press.connect(
            [this](MouseButtonEventContext &ev)
            {
                if (ev.button == 0x110)
                { // Left click
                    m_is_dragging = true;
                    m_mouse_press_x = ev.x;
                    m_drag_start_animated_index = m_animated_index;

                    // Stop any ongoing animation when user grabs the widget
                    if (m_animation_timer != 0 && application())
                    {
                        application()->stop_timer(m_animation_timer);
                        m_animation_timer = 0;
                    }
                }
            });

        when_mouse_drag.connect(
            [this](MouseMoveEventContext &ev)
            {
                if (m_is_dragging)
                {
                    int dx = (int)ev.x - m_mouse_press_x;
                    // Drag sensitivity: 200px moves one full item
                    float index_offset = -(float)dx / 200.0f;
                    m_animated_index = m_drag_start_animated_index + index_offset;

                    // Constrain to available items
                    m_animated_index =
                        std::max(0.0f, std::min((float)m_children.size() - 1, m_animated_index));

                    invalidate();
                    calculate_layout();
                }
            });

        when_mouse_release.connect(
            [this](MouseButtonEventContext &ev)
            {
                if (m_is_dragging)
                {
                    m_is_dragging = false;

                    // Snap to the nearest index
                    int nearest_index = (int)std::round(m_animated_index);
                    nearest_index =
                        std::max(0, std::min((int)m_children.size() - 1, nearest_index));

                    // Set selected index and trigger animation to snap
                    set_selected_index(nearest_index);

                    // If we were already at the nearest index, we still need to start a timer
                    // to ensure it perfectly aligns if it was off by a small fraction
                    if (m_selected_index == nearest_index &&
                        std::abs(m_animated_index - (float)nearest_index) > 0.001f)
                    {
                        if (m_animation_timer == 0 && application())
                        {
                            m_animation_timer = application()->add_timer(
                                16, [this]() { update_animation(); }, true);
                        }
                    }
                }
            });

        when_key_press.connect(
            [this](KeyEventContext &ev)
            {
                if (ev.keysym == 0xff51)
                { // Left arrow
                    set_selected_index(std::max(0, m_selected_index - 1));
                }
                else if (ev.keysym == 0xff53)
                { // Right arrow
                    set_selected_index(std::min((int)m_children.size() - 1, m_selected_index + 1));
                }
            });

        when_mouse_wheel.connect(
            [this](MouseWheelEventContext &ev)
            {
                if (ev.dy > 0)
                {
                    set_selected_index(std::min((int)m_children.size() - 1, m_selected_index + 1));
                }
                else if (ev.dy < 0)
                {
                    set_selected_index(std::max(0, m_selected_index - 1));
                }
                ev.stop_propagation = true;
            });

        set_focusable(true); // Allow keyboard input
    }

    CoverFlowBase::~CoverFlowBase()
    {
        if (m_animation_timer != 0 && application())
        {
            application()->stop_timer(m_animation_timer);
        }
        clear_cache();
    }

    void CoverFlowBase::clear_cache()
    {
        for (auto &pair : m_texture_cache)
        {
            if (pair.second.texture_id != 0 && m_app)
            {
                // We need to delete via application or queue if no GC is here.
                // For now, let's assume we can't easily get a GC here,
                // but we can queue it in the app.
                Application::GLDrawCall call;
                call.texture_id = pair.second.texture_id;
                call.delete_texture = true;
                call.opacity = -1.0f; // Convention for "just delete"
                m_app->queue_gl_draw(call);
            }
        }
        m_texture_cache.clear();
    }

    void CoverFlowBase::set_selected_index(int index)
    {
        if (index >= 0 && index < (int)m_children.size())
        {
            bool changed = (m_selected_index != index);
            m_selected_index = index;

            if (m_animated_index < 0.0f)
                m_animated_index = (float)index;

            // Only start animation if not dragging and we need to move
            if (!m_is_dragging && m_animation_timer == 0 && application() &&
                std::abs(m_animated_index - (float)index) > 0.001f)
            {
                m_animation_timer =
                    application()->add_timer(16, [this]() { update_animation(); }, true);
            }

            if (changed)
            {
                invalidate();
            }
        }
    }

    void CoverFlowBase::update_animation()
    {
        float diff = (float)m_selected_index - m_animated_index;
        if (std::abs(diff) < 0.01f)
        {
            m_animated_index = (float)m_selected_index;
            if (m_animation_timer != 0 && application())
            {
                application()->stop_timer(m_animation_timer);
                m_animation_timer = 0;
            }
            invalidate();
            return;
        }

        m_animated_index += diff * 0.15f;
        invalidate();
    }

    int CoverFlowBase::selected_index() const
    {
        return m_selected_index;
    }

    void CoverFlowBase::set_item_size(int width, int height)
    {
        m_item_width = width;
        m_item_height = height;
        invalidate();
    }

    void CoverFlowBase::calculate_layout()
    {
        Widget::calculate_layout();

        if (m_children.empty())
            return;

        // Calculate actual item size based on available space
        int actual_h = m_item_height;
        int actual_w = m_item_width;

        if (m_draw_reflection)
        {
            // If drawing reflections, ensure item + reflection fits.
            // 45% height for the item leaves 45% for reflection and 10% margin.
            int max_h = (int)(m_available_draw_height * 0.45f);
            if (actual_h > max_h)
            {
                float ratio = (float)actual_w / actual_h;
                actual_h = max_h;
                actual_w = (int)(max_h * ratio);
            }
        }
        else
        {
            int max_h = (int)(m_available_draw_height * 0.9f);
            if (actual_h > max_h)
            {
                float ratio = (float)actual_w / actual_h;
                actual_h = max_h;
                actual_w = (int)(max_h * ratio);
            }
        }

        int center_x = m_start_draw_x + m_available_draw_width / 2;
        // Perfect vertical center for the 'item + reflection' group
        int center_y = m_start_draw_y + m_available_draw_height / 2 - (actual_h / 2);

        for (int i = 0; i < (int)m_children.size(); ++i)
        {
            auto &child = m_children[i];
            float dist = (float)i - m_animated_index;

            // Adjust spacing to mimic dense CoverFlow layout
            float actual_spacing = actual_w * 0.12f;
            float actual_lateral_gap = actual_w * 0.35f;
            float clamped_dist = std::max(-1.0f, std::min(1.0f, dist));
            float x_offset = dist * actual_spacing + clamped_dist * actual_lateral_gap;

            child->set_size(actual_w, actual_h);
            int x = center_x - actual_w / 2 + (int)x_offset;
            int y = center_y;

            child->set_position(x, y);
        }
    }

    Widget *CoverFlowBase::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled)
            return nullptr;
        if (x < m_x || y < m_y || x >= m_x + m_width || y >= m_y + m_height)
            return nullptr;

        // Hit test in Z-order.
        // 1. Center
        if (m_selected_index >= 0 && m_selected_index < (int)m_children.size())
        {
            if (Widget *hit = m_children[m_selected_index]->hit_test(x, y))
                return hit;
        }

        // 2. Right side (closest to center first)
        for (int i = m_selected_index + 1; i < (int)m_children.size(); ++i)
        {
            if (Widget *hit = m_children[i]->hit_test(x, y))
                return hit;
        }

        // 3. Left side (closest to center first)
        for (int i = m_selected_index - 1; i >= 0; --i)
        {
            if (Widget *hit = m_children[i]->hit_test(x, y))
                return hit;
        }

        return this;
    }

    void CoverFlowBase::render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force)
    {
        if (!m_visible)
            return;

        calculate_layout();

        bool intersects =
            !(m_x >= cx + cw || m_x + m_width <= cx || m_y >= cy + ch || m_y + m_height <= cy);
        if (!intersects)
            return;

        bool should_draw = m_dirty || force || m_child_dirty;

        // --- 1. Draw Background ---
        if (should_draw)
        {
            gc.setColor(m_dark_mode ? Color(0.01f, 0.01f, 0.01f) : Color(0.99f, 0.99f, 0.99f));
            gc.fillRect(m_x, m_y, m_width, m_height);
        }

        if (!application() || !m_app)
            return;

        gc.save();
        gc.clip(m_x, m_y, m_width, m_height);

        // 1. Select visible/culled items and sort them
        const float CULL_THRESHOLD = 7.0f;
        std::vector<int> indices;
        for (int i = 0; i < (int)m_children.size(); i++)
        {
            float dist = std::abs((float)i - m_animated_index);
            if (dist < CULL_THRESHOLD)
            {
                indices.push_back(i);
            }
        }

        std::sort(indices.begin(), indices.end(),
                  [&](int a, int b)
                  {
                      double dist_a = std::abs((double)a - (double)m_animated_index);
                      double dist_b = std::abs((double)b - (double)m_animated_index);
                      return dist_a > dist_b; // Furthest first
                  });

        // Pre-calculate portal matrix: standard NDC maps to the widget's screen rectangle
        float portal[16];
        Matrix::identity(portal);
        float portal_x =
            (float)(m_x + m_width / 2.0f - m_app->width() / 2.0f) / (m_app->width() / 2.0f);
        float portal_y =
            -(float)(m_y + m_height / 2.0f - m_app->height() / 2.0f) / (m_app->height() / 2.0f);
        float portal_scale_x = (float)m_width / m_app->width();
        float portal_scale_y = (float)m_height / m_app->height();

        // Matrix::translate/scale are POST-multipliers (M = M * T).
        // We want Final = Translation * Scale * Perspective...
        // So we build T * S first and then multiply by projection.
        Matrix::translate(portal, portal_x, portal_y, 0.0f);
        Matrix::scale(portal, portal_scale_x, portal_scale_y, 1.0f);

        // 2. Render each visible child using the 3D path
        for (int i : indices)
        {
            Widget *child = m_children[i].get();
            double dist = (double)i - (double)m_animated_index;

            uint32_t tex_id = 0;
            int tex_w = 0, tex_h = 0;

            bool needs_rerender = (m_texture_cache.find(child) == m_texture_cache.end()) ||
                                  child->is_dirty() || child->is_child_dirty() || force;

            if (needs_rerender)
            {
                gc.save();
                gc.pushGroup();

                // Draw reflection
                if (m_draw_reflection)
                {
                    gc.save();
                    double mirror_y = child->y() + (double)child->height();
                    gc.translate(0, mirror_y);
                    gc.scale(1.0, -1.0);
                    gc.translate(0, -mirror_y);
                    gc.pushGroup();
                    child->render(gc, cx, cy, cw, ch, true);
                    gc.popGroupToSource();

                    gc.maskLinearGradient(child->x(), child->y(), child->width(), child->height(),
                                          Color(0.0f, 0.0f, 0.0f, 0.0f),
                                          Color(0.0f, 0.0f, 0.0f, 0.7f), true);
                    gc.restore();
                }

                // Draw main child
                child->render(gc, cx, cy, cw, ch, true);

                int capture_h = m_draw_reflection ? child->height() * 2 : child->height();
                gc.popGroupToTexture(tex_id, child->x(), child->y(), child->width(), capture_h);
                gc.restore();

                // Update cache
                if (m_texture_cache.count(child) && m_texture_cache[child].texture_id != 0)
                {
                    gc.deleteTexture(m_texture_cache[child].texture_id);
                }
                m_texture_cache[child] = {tex_id, child->width(), capture_h};
                tex_w = child->width();
                tex_h = capture_h;
            }
            else
            {
                const auto &cached = m_texture_cache[child];
                tex_id = cached.texture_id;
                tex_w = cached.width;
                tex_h = cached.height;
            }

            int capture_h = tex_h;
            // Calculate 3D Projection
            float mvp[16];
            Matrix::identity(mvp);

            float aspect = (float)m_width / m_height;
            float proj[16];
            Matrix::perspective(proj, 60.0f * 3.14159f / 180.0f, aspect, 0.1f, 100.0f);

            double pivot_x = child->x() + (double)child->width() / 2.0;
            double pivot_y = child->y() + (double)capture_h / 2.0;

            // Use widget-local coordinates for normalization
            double local_pivot_x = pivot_x - m_x;
            double local_pivot_y = pivot_y - m_y;

            // Dynamic depth and rotation mimicking classic Cover Flow
            float clamped_dist_rot = std::max(-1.0f, std::min(1.0f, (float)dist));
            float rotation = clamped_dist_rot * -1.1f;

            float depth_step = std::abs(clamped_dist_rot) * 0.6f;
            float z_pos = -2.0f - depth_step - (float)std::abs(dist) * 0.05f;

            float fov_rad = 60.0f * 3.14159f / 180.0f;
            float f_val = 1.0f / tanf(fov_rad / 2.0f);
            float screen_to_scene_x = std::abs(z_pos) * aspect / f_val;
            float screen_to_scene_y = std::abs(z_pos) / f_val;

            float norm_x = (float)(local_pivot_x - (m_width / 2.0f)) / (m_width / 2.0f);
            float norm_y = -(float)(local_pivot_y - (m_height / 2.0f)) / (m_height / 2.0f);
            float scene_x = norm_x * screen_to_scene_x;
            float scene_y = norm_y * screen_to_scene_y;

            float scene_scale_x = (float)child->width() / m_width * screen_to_scene_x;
            float scene_scale_y = (float)capture_h / m_height * screen_to_scene_y;

            Matrix::translate(mvp, scene_x, scene_y, z_pos);
            if (rotation != 0.0f)
            {
                Matrix::rotate_y(mvp, rotation);
            }

            Matrix::scale(mvp, scene_scale_x, scene_scale_y, 1.0f);

            // Apply portal transform last (pre-multiply logic)
            Matrix::multiply(mvp, proj, mvp);
            Matrix::multiply(mvp, portal, mvp);

            float opacity = 1.0f - std::min(0.7f, (float)std::abs(dist) * 0.15f);
            gc.drawTexture3D(tex_id, tex_w, tex_h, mvp, opacity, false);
        }

        gc.restore();
        m_dirty = false;
        m_child_dirty = false;
    }

} // namespace horizon
