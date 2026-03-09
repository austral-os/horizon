#include <cairo/cairo.h>
#include <cmath>
#include <horizon/Application.hpp>
#include <horizon/CoverFlow.hpp>
#include <horizon/GraphicsContext.hpp>

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

    void CoverFlowBase::set_selected_index(int index)
    {
        if (index >= 0 && index < (int)m_children.size() && m_selected_index != index)
        {
            m_selected_index = index;
            rebuild_items(); // trigger re-render of selected state to children
        }
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

        // Parameters for better 3D look
        float spacing = 55.0f;      // distance between side items
        float lateral_gap = 160.0f; // gap between center item and sides

        for (int i = 0; i < (int)m_children.size(); ++i)
        {
            auto &child = m_children[i];
            int dist = i - m_selected_index;

            float scale = 1.0f;
            float x_offset = 0.0f;

            if (dist < 0)
            {
                scale = 0.7f;
                x_offset = dist * spacing - lateral_gap;
            }
            else if (dist > 0)
            {
                scale = 0.7f;
                x_offset = dist * spacing + lateral_gap;
            }
            else
            {
                scale = 1.0f; // Center item is full size
            }

            int w = m_item_width * scale;
            int h = m_item_height * scale;

            child->set_size(w, h);
            int x = center_x - w / 2 + x_offset;
            // Align all items by their bottom edge to make reflections consistent
            int y = center_y + m_item_height / 2 - h;
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

        gc.save();
        gc.clip(m_x, m_y, m_width, m_height);

        auto draw_child_with_reflection = [&](int i)
        {
            auto &child = m_children[i];

            if (m_draw_reflection)
            {
                cairo_t *cr = static_cast<cairo_t *>(gc.getNativeContext());
                cairo_save(cr);

                // Mirror across the bottom of the widget
                double mirror_y = child->y() + child->height();

                // Translate to mirror line, flip vertically, translate back
                cairo_translate(cr, 0, 1.0 * mirror_y);
                cairo_scale(cr, 1.0, -1.0);
                cairo_translate(cr, 0, -1.0 * mirror_y);

                // Create a fade-out mask for reflection
                cairo_push_group(cr);
                // Draw child again as its own reflection
                child->render(gc, cx, cy, cw, ch, true);
                cairo_pop_group_to_source(cr);

                // Linear gradient mask for the fade effect
                // It should start quite transparent at the bottom of the widget
                cairo_pattern_t *mask =
                    cairo_pattern_create_linear(0, child->y(), 0, child->y() + child->height());
                cairo_pattern_add_color_stop_rgba(mask, 1.0, 0, 0, 0, 0.35); // Start of reflection
                cairo_pattern_add_color_stop_rgba(mask, 0.6, 0, 0, 0, 0.0);  // Fades out half-way

                cairo_mask(cr, mask);
                cairo_pattern_destroy(mask);
                cairo_restore(cr);
            }

            child->render(gc, cx, cy, cw, ch, should_draw);
        };

        // Draw left side (far to near)
        for (int i = 0; i < m_selected_index && i < (int)m_children.size(); ++i)
        {
            draw_child_with_reflection(i);
        }

        // Draw right side (far to near)
        for (int i = (int)m_children.size() - 1; i > m_selected_index; --i)
        {
            draw_child_with_reflection(i);
        }

        // Draw center
        if (m_selected_index >= 0 && m_selected_index < (int)m_children.size())
        {
            draw_child_with_reflection(m_selected_index);
        }

        gc.restore();

        m_dirty = false;
        m_child_dirty = false;
    }

} // namespace horizon
