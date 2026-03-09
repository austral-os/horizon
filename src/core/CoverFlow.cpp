#include <GLES2/gl2.h>
#include <cairo/cairo.h>
#include <cmath>
#include <horizon/Application.hpp>
#include <horizon/CoverFlow.hpp>
#include <horizon/GraphicsContext.hpp>

namespace horizon
{

    static void mat4_identity(float *m)
    {
        for (int i = 0; i < 16; i++)
            m[i] = 0;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static void mat4_multiply(float *out, const float *a, const float *b)
    {
        float res[16];
        for (int c = 0; c < 4; c++)
        {
            for (int r = 0; r < 4; r++)
            {
                res[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0] + a[1 * 4 + r] * b[c * 4 + 1] +
                                 a[2 * 4 + r] * b[c * 4 + 2] + a[3 * 4 + r] * b[c * 4 + 3];
            }
        }
        std::memcpy(out, res, 16 * sizeof(float));
    }

    void mat4_perspective(float *m, float fov, float aspect, float near, float far)
    {
        float f = 1.0f / tanf(fov / 2.0f);
        m[0] = f / aspect;
        m[1] = 0;
        m[2] = 0;
        m[3] = 0;
        m[4] = 0;
        m[5] = f;
        m[6] = 0;
        m[7] = 0;
        m[8] = 0;
        m[9] = 0;
        m[10] = (far + near) / (near - far);
        m[11] = -1;
        m[12] = 0;
        m[13] = 0;
        m[14] = (2.0f * far * near) / (near - far);
        m[15] = 0;
    }

    static void mat4_translate(float *m, float x, float y, float z)
    {
        float t[16];
        mat4_identity(t);
        t[12] = x;
        t[13] = y;
        t[14] = z;
        mat4_multiply(m, m, t);
    }

    static void mat4_rotate_y(float *m, float angle)
    {
        float r[16];
        mat4_identity(r);
        r[0] = cosf(angle);
        r[2] = sinf(angle);
        r[8] = -sinf(angle);
        r[10] = cosf(angle);
        mat4_multiply(m, m, r);
    }

    static void mat4_scale(float *m, float x, float y, float z)
    {
        float s[16];
        mat4_identity(s);
        s[0] = x;
        s[5] = y;
        s[10] = z;
        mat4_multiply(m, m, s);
    }

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
            if (pair.second.texture_id != 0)
            {
                glDeleteTextures(1, &pair.second.texture_id);
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

        int center_x = m_start_draw_x + m_available_draw_width / 2;
        // Shift center_y up slightly to account for reflections at the bottom
        int center_y = m_start_draw_y + m_available_draw_height / 2 -
                       (m_draw_reflection ? (int)(m_item_height / 4.0f) : 0);

        // Parameters for 3D look
        float spacing = 25.0f;      // Closer spacing for side items
        float lateral_gap = 130.0f; // Larger gap around the center item

        for (int i = 0; i < (int)m_children.size(); ++i)
        {
            auto &child = m_children[i];
            float dist = (float)i - m_animated_index;
            float abs_dist = std::abs(dist);

            // To mimic CoverFlow geometry, items maintain their full resolution
            // and perspective is handled completely by 3D mapping.
            float scale = 1.0f;

            float clamped_dist = std::max(-1.0f, std::min(1.0f, dist));

            // Adjust spacing to mimic dense CoverFlow layout
            // Side items are close together, center has a larger gap
            float actual_spacing = m_item_width * 0.12f;
            float actual_lateral_gap = m_item_width * 0.35f;
            float x_offset = dist * actual_spacing + clamped_dist * actual_lateral_gap;

            int w = (int)(m_item_width * scale);
            int h = (int)(m_item_height * scale);

            child->set_size(w, h);
            int x = center_x - w / 2 + (int)x_offset;

            // Vertically align perfectly centered
            int y = center_y - (h / 2);

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
            // Even darker background for more contrast
            gc.setColor(m_dark_mode ? Color(0.01f, 0.01f, 0.01f) : Color(0.99f, 0.99f, 0.99f));
            gc.fillRect(m_x, m_y, m_width, m_height);
        }

        if (!application() || !m_app)
            return;

        cairo_t *cr = static_cast<cairo_t *>(gc.getNativeContext());
        gc.save();
        gc.clip(m_x, m_y, m_width, m_height);

        // 1. Select visible/culled items and sort them
        // Items further than CULL_THRESHOLD from m_animated_index are skipped
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

        // 2. Render each visible child using the 3D path
        for (int i : indices)
        {
            Widget *child = m_children[i].get();
            double dist = (double)i - (double)m_animated_index;

            uint32_t tex_id = 0;
            int tex_w = 0;
            int tex_h = 0;

            // Texture Caching: Only re-render if the widget or its children are dirty,
            // or if it's missing from the cache.
            bool needs_rerender = (m_texture_cache.find(child) == m_texture_cache.end()) ||
                                  child->is_dirty() || child->is_child_dirty() || force;

            if (needs_rerender)
            {
                cairo_save(cr);
                gc.pushGroup();

                // Draw reflection
                if (m_draw_reflection)
                {
                    cairo_save(cr);
                    double mirror_y = child->y() + (double)child->height();
                    cairo_translate(cr, 0, mirror_y);
                    cairo_scale(cr, 1.0, -1.0);
                    cairo_translate(cr, 0, -mirror_y);
                    cairo_push_group(cr);
                    child->render(gc, cx, cy, cw, ch, true);
                    cairo_pop_group_to_source(cr);
                    cairo_pattern_t *mask =
                        cairo_pattern_create_linear(0, child->y(), 0, child->y() + child->height());
                    float offset_start = 1.0f;
                    float offset_end = 0.5f;
                    cairo_pattern_add_color_stop_rgba(mask, offset_start, 0, 0, 0, 0.4);
                    cairo_pattern_add_color_stop_rgba(mask, offset_end, 0, 0, 0, 0.0);
                    cairo_mask(cr, mask);
                    cairo_pattern_destroy(mask);
                    cairo_restore(cr);
                }

                // Draw main child
                child->render(gc, cx, cy, cw, ch, true);

                int capture_h = m_draw_reflection ? child->height() * 2 : child->height();
                gc.popGroupToTexture(tex_id, child->x(), child->y(), child->width(), capture_h);
                cairo_restore(cr);

                // Update cache (deleting old texture if it exists)
                if (m_texture_cache.count(child) && m_texture_cache[child].texture_id != 0)
                {
                    glDeleteTextures(1, &m_texture_cache[child].texture_id);
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
            mat4_identity(mvp);

            float aspect = (float)m_app->width() / m_app->height();
            float proj[16];
            mat4_perspective(proj, 100.0f * 3.14159f / 180.0f, aspect, 0.1f, 100.0f);

            double pivot_x = child->x() + (double)child->width() / 2.0;
            double pivot_y = child->y() + (double)capture_h / 2.0;

            // Dynamic depth and rotation mimicking classic Cover Flow
            float clamped_dist_rot = std::max(-1.0f, std::min(1.0f, (float)dist));
            float rotation = clamped_dist_rot * -1.75f;

            // Push side items back to a common depth plane, with tiny progressive offset
            float depth_step = std::abs(clamped_dist_rot) * 0.6f;
            float z_pos = -1.0f - depth_step - (float)std::abs(dist) * 0.05f;

            float fov_rad = 100.0f * 3.14159f / 180.0f;
            float f_val = 1.0f / tanf(fov_rad / 2.0f);
            float screen_to_scene_x = std::abs(z_pos) * aspect / f_val;
            float screen_to_scene_y = std::abs(z_pos) / f_val;

            float norm_x = (float)(pivot_x - (m_app->width() / 2.0f)) / (m_app->width() / 2.0f);
            float norm_y = -(float)(pivot_y - (m_app->height() / 2.0f)) / (m_app->height() / 2.0f);
            float scene_x = norm_x * screen_to_scene_x;
            float scene_y = norm_y * screen_to_scene_y;

            float scene_scale_x = (float)child->width() / m_app->width() * screen_to_scene_x;
            float scene_scale_y = (float)capture_h / m_app->height() * screen_to_scene_y;

            mat4_translate(mvp, scene_x, scene_y, z_pos);
            if (rotation != 0.0f)
            {
                mat4_rotate_y(mvp, rotation);
            }

            mat4_scale(mvp, scene_scale_x, scene_scale_y, 1.0f);
            mat4_multiply(mvp, proj, mvp);

            float opacity = 1.0f - std::min(0.7f, (float)std::abs(dist) * 0.15f);
            gc.drawTexture3D(tex_id, tex_w, tex_h, mvp, opacity, false);
        }

        gc.restore();
        m_dirty = false;
        m_child_dirty = false;
    }

} // namespace horizon
