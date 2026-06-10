#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/HPanel.hpp>

namespace horizon
{
    HPanel::HPanel() : Widget()
    {
        set_focusable(true);
        m_divider_height = 2; // Match VPanel's default width

        when_mouse_move.connect(
            [this](MouseMoveEventContext &ev)
            {
                if (m_is_resizing)
                    return;

                if (is_over_divider((int)ev.x, (int)ev.y))
                {
                    set_cursor_type(CursorType::ResizeNS);
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
                    set_cursor_type(CursorType::ResizeNS);
                    ev.stop_propagation = true;
                }
            });

        when_mouse_drag.connect(
            [this](MouseMoveEventContext &ev)
            {
                if (m_is_resizing)
                {
                    int local_y = (int)(ev.y - m_y);
                    m_top_height = std::clamp(local_y, 10, m_height - 10);
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

    void HPanel::add_child(std::unique_ptr<Widget> child)
    {
        if (m_children.size() >= 2)
        {
            return;
        }

        Widget *ptr = child.get();
        if (m_children.empty())
        {
            m_top_ptr = ptr;
        }
        else
        {
            m_bottom_ptr = ptr;
        }

        child->set_position_type(FREE);
        Widget::add_child(std::move(child));
    }

    Widget *HPanel::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled)
            return nullptr;
        if (x < m_x || y < m_y || x >= m_x + m_width || y >= m_y + m_height)
            return nullptr;

        if (is_over_divider(x, y))
            return this;

        return Widget::hit_test(x, y);
    }

    void HPanel::calculate_layout()
    {
        bool top_visible = m_top_ptr && m_top_ptr->is_visible();
        bool bottom_visible = m_bottom_ptr && m_bottom_ptr->is_visible();

        if (!top_visible && bottom_visible)
        {
            m_bottom_ptr->set_position(m_x, m_y);
            m_bottom_ptr->set_size(m_width, m_height);
            return;
        }

        if (top_visible && !bottom_visible)
        {
            m_top_ptr->set_position(m_x, m_y);
            m_top_ptr->set_size(m_width, m_height);
            return;
        }

        int divider_y = m_y + m_top_height;

        if (m_top_ptr)
        {
            m_top_ptr->set_position(m_x, m_y);
            m_top_ptr->set_size(m_width, m_top_height - (m_divider_height / 2));
        }

        if (m_bottom_ptr)
        {
            int bottom_start = divider_y + (m_divider_height / 2);
            m_bottom_ptr->set_position(m_x, bottom_start);
            m_bottom_ptr->set_size(m_width, m_y + m_height - bottom_start);
        }
    }

    void HPanel::draw(GraphicsContext &gc)
    {
        auto *tm = theme_manager();
        Color bg_color = tm->get_color("window_bg");

        gc.setColor(bg_color);
        gc.fillRect(m_x, m_y, m_width, m_height);
    }

    bool HPanel::is_over_divider(int cursor_x, int cursor_y) const
    {
        int divider_center_y = m_y + m_top_height;
        int y_min = divider_center_y - (m_divider_height / 2) - 4;
        int y_max = divider_center_y + (m_divider_height / 2) + 4;

        return (cursor_y >= y_min && cursor_y <= y_max && cursor_x >= m_x &&
                cursor_x <= m_x + m_width);
    }

    void HPanel::set_top_height(int height)
    {
        m_top_height = height;
        invalidate();
    }

    int HPanel::top_height() const
    {
        return m_top_height;
    }

} // namespace horizon
