#include <horizon/Widget.hpp>
#include <iostream>

namespace horizon
{

    Widget::Widget() {}

    Widget::~Widget() {}

    void Widget::render(GraphicsContext &ctx)
    {
        if (!m_visible)
            return;

        draw(ctx);

        for (const auto &child : m_children)
        {
            std::cout << "Rendering child" << std::endl;
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