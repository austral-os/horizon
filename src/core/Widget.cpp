#include <horizon/Widget.hpp>
#include <linux/input-event-codes.h>

namespace horizon
{

    Widget::Widget()
    {
        m_layout_type = WIDGET_LAYOUT_VERTICAL;
        m_position_type = FILL;
        set_cursor_type(CursorType::Default);
    }

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
            m_free_space = m_available_draw_height - (m_margin * 2);
        }
        else
        {
            m_free_space = m_available_draw_width - (m_margin * 2);
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

        m_free_space -= (m_margin * m_free_children_count);
    }

    void Widget::render(GraphicsContext &ctx)
    {
        if (!m_visible)
            return;

        calculate_layout();

        draw(ctx);

        int current_x = m_start_draw_x;
        int current_y = m_start_draw_y;

        if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
            current_y += m_margin;
        else
            current_x += m_margin;

        for (const auto &child : m_children)
        {
            if (child->fixed_size() > 0)
            {
                if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_available_draw_width, child->fixed_size());
                    current_y += child->fixed_size() + m_margin;
                }
                else
                {
                    child->set_position(current_x, current_y);
                    child->set_size(child->fixed_size(), m_available_draw_height);
                    current_x += child->fixed_size() + m_margin;
                }
            }
            else
            {
                if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_available_draw_width, m_free_space / m_free_children_count);
                    current_y += m_free_space / m_free_children_count + m_margin;
                }
                else
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_free_space / m_free_children_count, m_available_draw_height);
                    current_x += m_free_space / m_free_children_count + m_margin;
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
            if (Widget *hit = child->hit_test(x, y))
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
        child->m_app = m_app; // Propage application pointer
        m_children.push_back(std::move(child));
    }

    Widget *Widget::parent() const
    {
        return m_parent;
    }

    Application *Widget::application() const
    {
        if (m_app)
            return m_app;
        if (m_parent)
            return m_parent->application();
        return nullptr;
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

    void Widget::set_cursor_type(CursorType type)
    {
        m_cursor_type = type;
    }

    CursorType Widget::cursor_type() const
    {
        return m_cursor_type;
    }

    void Widget::set_on_click(std::function<void()> handler)
    {
        m_on_mouse_press_handlers.clear();
        if (handler)
        {
            add_on_click(handler);
        }
    }

    // Mouse Enter
    size_t Widget::add_on_mouse_enter(std::function<void()> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_mouse_enter_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_mouse_enter(size_t id)
    {
        m_on_mouse_enter_handlers.erase(id);
    }

    // Mouse Leave
    size_t Widget::add_on_mouse_leave(std::function<void()> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_mouse_leave_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_mouse_leave(size_t id)
    {
        m_on_mouse_leave_handlers.erase(id);
    }

    // Mouse Move
    size_t Widget::add_on_mouse_move(std::function<void(int, int)> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_mouse_move_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_mouse_move(size_t id)
    {
        m_on_mouse_move_handlers.erase(id);
    }

    // Mouse Press
    size_t Widget::add_on_mouse_press(std::function<void(int)> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_mouse_press_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_mouse_press(size_t id)
    {
        m_on_mouse_press_handlers.erase(id);
    }

    // Mouse Release
    size_t Widget::add_on_mouse_release(std::function<void(int)> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_mouse_release_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_mouse_release(size_t id)
    {
        m_on_mouse_release_handlers.erase(id);
    }

    // Mouse Drag
    size_t Widget::add_on_mouse_drag(std::function<void(int, int)> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_mouse_drag_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_mouse_drag(size_t id)
    {
        m_on_mouse_drag_handlers.erase(id);
    }

    // Mouse Hover
    size_t Widget::add_on_mouse_hover(std::function<void(int, int)> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_mouse_hover_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_mouse_hover(size_t id)
    {
        m_on_mouse_hover_handlers.erase(id);
    }

    // Key Press
    size_t Widget::add_on_key_press(std::function<void(int)> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_key_press_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_key_press(size_t id)
    {
        m_on_key_press_handlers.erase(id);
    }

    // Key Release
    size_t Widget::add_on_key_release(std::function<void(int)> handler)
    {
        size_t id = m_next_handler_id++;
        m_on_key_release_handlers[id] = handler;
        return id;
    }
    void Widget::remove_on_key_release(size_t id)
    {
        m_on_key_release_handlers.erase(id);
    }

    void Widget::draw(GraphicsContext &) {}

    void Widget::on_mouse_enter()
    {
        for (auto const &[id, handler] : m_on_mouse_enter_handlers)
            if (handler)
                handler();
    }
    void Widget::on_mouse_leave()
    {
        for (auto const &[id, handler] : m_on_mouse_leave_handlers)
            if (handler)
                handler();
    }
    void Widget::on_mouse_move(int x, int y)
    {
        for (auto const &[id, handler] : m_on_mouse_move_handlers)
            if (handler)
                handler(x, y);
    }
    void Widget::on_mouse_press(int button)
    {
        for (auto const &[id, handler] : m_on_mouse_press_handlers)
            if (handler)
                handler(button);
    }
    void Widget::on_mouse_release(int button)
    {
        for (auto const &[id, handler] : m_on_mouse_release_handlers)
            if (handler)
                handler(button);
    }
    void Widget::on_key_press(int key)
    {
        for (auto const &[id, handler] : m_on_key_press_handlers)
            if (handler)
                handler(key);
    }
    void Widget::on_key_release(int key)
    {
        for (auto const &[id, handler] : m_on_key_release_handlers)
            if (handler)
                handler(key);
    }
    void Widget::on_mouse_drag(int x, int y)
    {
        for (auto const &[id, handler] : m_on_mouse_drag_handlers)
            if (handler)
                handler(x, y);
    }
    void Widget::on_mouse_hover(int x, int y)
    {
        for (auto const &[id, handler] : m_on_mouse_hover_handlers)
            if (handler)
                handler(x, y);
    }

} // namespace horizon