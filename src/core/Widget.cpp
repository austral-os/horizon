#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>
#include <linux/input-event-codes.h>

namespace horizon
{

    Widget::Widget()
    {
        m_layout_type = WIDGET_LAYOUT_VERTICAL;
        m_position_type = FILL;
        set_cursor_type(CursorType::Default);
        set_accent_color(WidgetAccentColor::Default);

        // Gestión de estados de interacción
        when_mouse_enter.connect(
            [this](EventContext &)
            {
                m_is_hovered = true;
                // invalidate(); //redibuja el widget
            });
        when_mouse_leave.connect(
            [this](EventContext &)
            {
                m_is_hovered = false;
                // invalidate();
            });
        when_mouse_press.connect(
            [this](EventContext &)
            {
                m_is_pressed = true;
                // invalidate();
            });
        when_mouse_release.connect(
            [this](EventContext &)
            {
                m_is_pressed = false;
                // invalidate();
            });
    }

    Widget::~Widget() {}

    void Widget::calculate_layout()
    {
        int count_non_free = 0;
        m_free_children_count = 0;
        m_start_draw_x = m_x + m_margin;
        m_start_draw_y = m_y + m_margin;
        m_available_draw_width = m_width - (m_margin * 2);
        m_available_draw_height = m_height - (m_margin * 2);
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

            if (!child->is_visible())
                continue;

            if (child->position_type() == FREE)
                continue;

            if (child->fixed_size() > 0)
            {
                m_free_space -= child->fixed_size();
            }
            else
            {
                m_free_children_count++;
            }

            count_non_free++; // Cuenta los widgets del tipo FILL
        }

        if (count_non_free > 1)
        {
            m_free_space -= (m_spacing * (count_non_free - 1));
        }
    }

    void Widget::render(GraphicsContext &ctx)
    {
        if (!m_visible)
            return;

        calculate_layout();

        ctx.pushGroup();
        draw(ctx);
        ctx.popGroup();

        int current_x = m_start_draw_x;
        int current_y = m_start_draw_y;

        for (const auto &child : m_children)
        {
            if (child->position_type() == FREE)
            {
                continue;
            }

            if (!child->is_visible())
                continue;

            if (child->fixed_size() > 0)
            {
                if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_available_draw_width, child->fixed_size());
                    current_y += child->fixed_size() + m_spacing;
                }
                else
                {
                    child->set_position(current_x, current_y);
                    child->set_size(child->fixed_size(), m_available_draw_height);
                    current_x += child->fixed_size() + m_spacing;
                }
            }
            else
            {
                if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_available_draw_width, m_free_space / m_free_children_count);
                    current_y += m_free_space / m_free_children_count + m_spacing;
                }
                else
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_free_space / m_free_children_count, m_available_draw_height);
                    current_x += m_free_space / m_free_children_count + m_spacing;
                }
            }

            // std::cout << "Rendering child" << std::endl;
            child->render(ctx);
        }

        for (const auto &child : m_children)
        {
            if (child->position_type() == FREE)
            {
                child->render(ctx);
            }
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
        child->set_application_recursive(m_app);
        m_children.push_back(std::move(child));
    }

    void Widget::add_child_at(int index, std::unique_ptr<Widget> child)
    {
        if (!child)
            return;

        child->m_parent = this;
        child->set_application_recursive(m_app);
        m_children.insert(m_children.begin() + index, std::move(child));
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

    void Widget::set_spacing(int spacing)
    {
        m_spacing = spacing;
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

    void Widget::set_accent_color(WidgetAccentColor accent_color)
    {
        m_accent_color = accent_color;
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

    int Widget::spacing() const
    {
        return m_spacing;
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

    WidgetAccentColor Widget::accent_color() const
    {
        return m_accent_color;
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

    void Widget::set_focus(bool focus)
    {
        if (m_has_focus != focus)
        {
            m_has_focus = focus;
            invalidate();
        }
    }

    bool Widget::is_hovered() const
    {
        return m_is_hovered;
    }

    bool Widget::is_pressed() const
    {
        return m_is_pressed;
    }

    void Widget::set_cursor_type(CursorType type)
    {
        m_cursor_type = type;
    }

    CursorType Widget::cursor_type() const
    {
        return m_cursor_type;
    }

    void Widget::draw(GraphicsContext &) {}

    void Widget::set_application_recursive(Application *app)
    {
        m_app = app;
        for (auto &child : m_children)
        {
            child->set_application_recursive(app);
        }
    }

    void Widget::invalidate()
    {
        Application *app = application();
        if (app)
        {
            app->invalidate(this);
        }
    }

} // namespace horizon
