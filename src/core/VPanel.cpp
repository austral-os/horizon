#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/VPanel.hpp>

namespace horizon
{
    VPanel::VPanel() : Widget()
    {
        set_focusable(true);
        m_divider_width = 2;

        when_mouse_move.connect(
            [this](MouseMoveEventContext &ev)
            {
                if (m_is_resizing)
                    return;

                if (is_over_divider((int)ev.x, (int)ev.y))
                {
                    set_cursor_type(CursorType::ResizeEW);
                }
                else
                {
                    set_cursor_type(CursorType::Default);
                }
            });

        when_mouse_press.connect(
            [this](MouseButtonEventContext &ev)
            {
                if (is_over_divider((int)ev.x, (int)ev.y))
                {
                    m_is_resizing = true;
                    set_cursor_type(CursorType::ResizeEW);
                    ev.stop_propagation = true;
                }
            });

        when_mouse_drag.connect(
            [this](MouseMoveEventContext &ev)
            {
                if (m_is_resizing)
                {
                    int local_x = (int)(ev.x - m_x);
                    m_left_width = std::clamp(local_x, 10, m_width - 10);
                    invalidate();
                    calculate_layout();
                }
            });

        when_mouse_release.connect(
            [this](MouseButtonEventContext &ev)
            {
                if (m_is_resizing)
                {
                    m_is_resizing = false;
                    set_cursor_type(CursorType::Default);
                }
            });
    }

    void VPanel::add_child(std::unique_ptr<Widget> child)
    {
        if (m_children.size() >= 2)
        {
            return;
        }

        Widget *ptr = child.get();
        if (m_children.empty())
        {
            m_left_ptr = ptr;
        }
        else
        {
            m_right_ptr = ptr;
        }

        child->set_position_type(FREE);
        Widget::add_child(std::move(child));
    }

    Widget *VPanel::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled)
            return nullptr;
        if (x < m_x || y < m_y || x >= m_x + m_width || y >= m_y + m_height)
            return nullptr;

        if (is_over_divider(x, y))
            return this;

        return Widget::hit_test(x, y);
    }

    void VPanel::calculate_layout()
    {
        bool left_visible = m_left_ptr && m_left_ptr->is_visible();
        bool right_visible = m_right_ptr && m_right_ptr->is_visible();

        if (!left_visible && right_visible)
        {
            m_right_ptr->set_position(m_x, m_y);
            m_right_ptr->set_size(m_width, m_height);
            return;
        }

        if (left_visible && !right_visible)
        {
            m_left_ptr->set_position(m_x, m_y);
            m_left_ptr->set_size(m_width, m_height);
            return;
        }

        int divider_x = m_x + m_left_width;

        if (m_left_ptr)
        {
            m_left_ptr->set_position(m_x, m_y);
            m_left_ptr->set_size(m_left_width - (m_divider_width / 2), m_height);
        }

        if (m_right_ptr)
        {
            int right_start = divider_x + (m_divider_width / 2);
            m_right_ptr->set_position(right_start, m_y);
            m_right_ptr->set_size(m_x + m_width - right_start, m_height);
        }
    }

    void VPanel::draw(GraphicsContext &gc)
    {
        gc.setColor(0.9f, 0.9f, 0.9f, 1.0f);
        gc.fillRect(m_x, m_y, m_width, m_height);
    }

    bool VPanel::is_over_divider(int cursor_x, int cursor_y) const
    {
        int divider_center_x = m_x + m_left_width;
        int x_min = divider_center_x - (m_divider_width / 2) - 4;
        int x_max = divider_center_x + (m_divider_width / 2) + 4;

        return (cursor_x >= x_min && cursor_x <= x_max && cursor_y >= m_y &&
                cursor_y <= m_y + m_height);
    }

    void VPanel::set_left_width(int width)
    {
        m_left_width = width;
        invalidate();
    }

    int VPanel::left_width() const
    {
        return m_left_width;
    }

} // namespace horizon
