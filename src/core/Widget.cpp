#include <horizon/Widget.hpp>
#include <iostream>

namespace horizon
{

    Widget::Widget() {}

    Widget::~Widget() {}

    void Widget::calculate_layout()
    {
        m_free_children_count = 0;
        m_start_draw_x = m_x + m_padding;
        m_start_draw_y = m_y + m_padding;
        m_available_draw_width = m_width - (m_padding * 2);
        m_available_draw_height = m_height - (m_padding * 2);
        if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
        {
            m_free_space = m_available_draw_height;
        }
        else
        {
            m_free_space = m_available_draw_width;
        }

        // recorro los hijos y si tienen fixedSize los resto del espacio disponible
        for (const auto &child : m_children)
        {
            if (child->fixed_size() > 0)
            {
                m_free_space -= child->fixed_size();
            }
            else
            {
                m_free_children_count++;
            }
        }
    }

    void Widget::render(GraphicsContext &ctx)
    {
        if (!m_visible)
            return;

        calculate_layout();

        draw(ctx);

        int current_x = m_start_draw_x;
        int current_y = m_start_draw_y;

        for (const auto &child : m_children)
        {
            if (child->fixed_size() > 0)
            {
                if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_available_draw_width, child->fixed_size());
                    current_y += child->fixed_size();
                }
                else
                {
                    child->set_position(current_x, current_y);
                    child->set_size(child->fixed_size(), m_available_draw_height);
                    current_x += child->fixed_size();
                }
            }
            else
            {
                if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_available_draw_width, m_free_space / m_free_children_count);
                    current_y += m_free_space / m_free_children_count;
                }
                else
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_free_space / m_free_children_count, m_available_draw_height);
                    current_x += m_free_space / m_free_children_count;
                }
            }

            // std::cout << "Rendering child" << std::endl;
            child->render(ctx);
        }
    }

    Widget *Widget::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled)
            return nullptr;

        // ¿El punto está dentro de este widget?
        if (x < m_x || y < m_y || x >= m_x + m_width || y >= m_y + m_height)
        {
            return nullptr;
        }

        // Convertimos a coordenadas locales
        int local_x = x - m_x;
        int local_y = y - m_y;

        // Recorremos hijos en orden inverso (top-most primero)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
        {
            Widget *child = it->get();
            if (Widget *hit = child->hit_test(local_x, local_y))
                return hit;
        }

        // Si ningún hijo lo contiene, este widget es el hit
        return this;
    }

    void Widget::add_child(std::unique_ptr<Widget> child)
    {
        if (!child)
            return;

        child->m_parent = this;
        m_children.push_back(std::move(child));
    }

    Widget *Widget::parent() const
    {
        return m_parent;
    }

    const std::vector<std::unique_ptr<Widget>> &Widget::children() const
    {
        return m_children;
    }

    void Widget::set_position(int x, int y)
    {
        m_x = x;
        m_y = y;
    }

    void Widget::set_size(int width, int height)
    {
        m_width = width;
        m_height = height;
    }

    void Widget::set_fixed_size(int size)
    {
        m_fixed_size = size;
    }

    void Widget::set_padding(int padding)
    {
        m_padding = padding;
    }

    void Widget::set_margin(int margin)
    {
        m_margin = margin;
    }

    void Widget::set_position_type(WidgetPositionTypes position_type)
    {
        m_position_type = position_type;
    }

    void Widget::set_layout_type(WidgetLayoutTypes layout_type)
    {
        m_layout_type = layout_type;
    }

    int Widget::x() const
    {
        return m_x;
    }
    int Widget::y() const
    {
        return m_y;
    }
    int Widget::width() const
    {
        return m_width;
    }
    int Widget::height() const
    {
        return m_height;
    }

    int Widget::fixed_size() const
    {
        return m_fixed_size;
    }

    int Widget::padding() const
    {
        return m_padding;
    }

    int Widget::margin() const
    {
        return m_margin;
    }

    WidgetPositionTypes Widget::position_type() const
    {
        return m_position_type;
    }

    WidgetLayoutTypes Widget::layout_type() const
    {
        return m_layout_type;
    }

    void Widget::set_visible(bool visible)
    {
        m_visible = visible;
    }

    bool Widget::is_visible() const
    {
        return m_visible;
    }

    void Widget::set_enabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool Widget::is_enabled() const
    {
        return m_enabled;
    }

    void Widget::set_focusable(bool focusable)
    {
        m_focusable = focusable;
    }

    bool Widget::is_focusable() const
    {
        return m_focusable;
    }

    bool Widget::has_focus() const
    {
        return m_has_focus;
    }

    void Widget::draw(GraphicsContext &) {}

    void Widget::on_mouse_enter() {}
    void Widget::on_mouse_leave() {}
    void Widget::on_mouse_move(int, int) {}
    void Widget::on_mouse_press(int) {}
    void Widget::on_mouse_release(int) {}
    void Widget::on_key_press(int key)
    {
        std::cout << "Key pressed: " << key << std::endl;
    }
    void Widget::on_key_release(int key)
    {
        std::cout << "Key released: " << key << std::endl;
    }
    void Widget::on_mouse_drag(int, int) {}
    void Widget::on_mouse_hover(int, int) {}

} // namespace horizon