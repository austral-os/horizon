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
                    m_drag_start_index = m_selected_index;
                }
            });

        when_mouse_move.connect(
            [this](MouseMoveEventContext &ev)
            {
                if (m_is_dragging)
                {
                    int dx = ev.x - m_mouse_press_x;
                    // drag distance threshold
                    int index_offset = -dx / 100; // change index every 100px
                    int next_index = m_drag_start_index + index_offset;
                    next_index = std::max(0, std::min((int)m_children.size() - 1, next_index));
                    if (next_index != m_selected_index)
                    {
                        set_selected_index(next_index);
                    }
                }
            });

        when_mouse_release.connect(
            [this](MouseButtonEventContext &ev)
            {
                if (ev.button == 0x110)
                {
                    m_is_dragging = false;
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

        set_focusable(true); // Allow keyboard input
    }

    CoverFlowBase::~CoverFlowBase()
    {
        if (m_animation_timer != 0 && application())
        {
            application()->stop_timer(m_animation_timer);
        }
    }

    void CoverFlowBase::set_selected_index(int index)
    {
        if (index >= 0 && index < (int)m_children.size() && m_selected_index != index)
        {
            m_selected_index = index;
            if (m_animated_index < 0.0f)
                m_animated_index = (float)index;

            if (m_animation_timer == 0 && application())
            {
                m_animation_timer =
                    application()->add_timer(16, [this]() { update_animation(); }, true);
            }
            rebuild_items(); // trigger re-render of selected state to children
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

        // 1. Determine which child is "center"
        Widget *center_child = (m_selected_index >= 0 && m_selected_index < (int)m_children.size())
                                   ? m_children[m_selected_index].get()
                                   : nullptr;

        // 2. Sort children for correct back-to-front rendering
        // We draw further items first (leftmost and rightmost)
        std::vector<int> indices;
        for (int i = 0; i < (int)m_children.size(); i++)
            indices.push_back(i);

        std::sort(indices.begin(), indices.end(),
                  [&](int a, int b)
                  {
                      double dist_a = std::abs((double)a - (double)m_animated_index);
                      double dist_b = std::abs((double)b - (double)m_animated_index);
                      return dist_a > dist_b; // Furthest first
                  });

        // 3. Render each child using the 3D path
        for (int i : indices)
        {
            Widget *child = m_children[i].get();
            double dist = (double)i - (double)m_animated_index;

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
                // Make the reflection start much stronger (0.8 alpha) and fade out slower (0.4)
                cairo_pattern_add_color_stop_rgba(mask, 1.0, 0, 0, 0, 0.8);
                cairo_pattern_add_color_stop_rgba(mask, 0.4, 0, 0, 0, 0.0);
                cairo_mask(cr, mask);
                cairo_pattern_destroy(mask);
                cairo_restore(cr);
            }

            // Draw main child
            child->render(gc, cx, cy, cw, ch, true);

            int capture_h = m_draw_reflection ? child->height() * 2 : child->height();
            uint32_t tex_id = 0;
            gc.popGroupToTexture(tex_id, child->x(), child->y(), child->width(), capture_h);

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
            // Steeper angle for side items
            float rotation = clamped_dist_rot * -1.75f;

            // Push side items back to a common depth plane, with tiny progressive offset
            float depth_step = std::abs(clamped_dist_rot) * 0.6f;
            float z_pos = -1.0f - depth_step - (float)std::abs(dist) * 0.05f;

            // 4. Calculate Screen-to-Scene mapping at this depth
            // This ensures quads perfectly match their 2D positions/sizes
            float fov_rad = 100.0f * 3.14159f / 180.0f;
            float f_val = 1.0f / tanf(fov_rad / 2.0f);
            float screen_to_scene_x = std::abs(z_pos) * aspect / f_val;
            float screen_to_scene_y = std::abs(z_pos) / f_val;

            // Convert normalized screen coordinates to scene coordinates
            float norm_x = (float)(pivot_x - (m_app->width() / 2.0f)) / (m_app->width() / 2.0f);
            float norm_y = -(float)(pivot_y - (m_app->height() / 2.0f)) / (m_app->height() / 2.0f);
            float scene_x = norm_x * screen_to_scene_x;
            float scene_y = norm_y * screen_to_scene_y;

            // Convert normalized screen size to scene size
            float scene_scale_x = (float)child->width() / m_app->width() * screen_to_scene_x;
            float scene_scale_y = (float)capture_h / m_app->height() * screen_to_scene_y;

            mat4_translate(mvp, scene_x, scene_y, z_pos);
            if (rotation != 0.0f)
            {
                mat4_rotate_y(mvp, rotation);
            }

            mat4_scale(mvp, scene_scale_x, scene_scale_y, 1.0f);
            mat4_multiply(mvp, proj, mvp);

            // Dynamic opacity based on distance
            float opacity = 1.0f - std::min(0.7f, (float)std::abs(dist) * 0.15f);
            gc.drawTexture3D(tex_id, child->width(), capture_h, mvp, opacity);

            cairo_restore(cr);
        }

        gc.restore();
        m_dirty = false;
        m_child_dirty = false;
    }

} // namespace horizon
