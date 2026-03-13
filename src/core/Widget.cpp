#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Window.hpp>
#include <linux/input-event-codes.h>

namespace horizon
{

    Widget::Widget()
    {
        m_layout_type = WIDGET_LAYOUT_VERTICAL;
        m_position_type = FILL;
        set_cursor_type(CursorType::Default);
        set_accent_color(WidgetAccentColor::Default);
        set_draw_state(WidgetDrawState::NORMAL);

        map_draw_state(WidgetEvent::MOUSE_ENTER, WidgetDrawState::HOVERED);
        map_draw_state(WidgetEvent::MOUSE_LEAVE, WidgetDrawState::NORMAL);
        map_draw_state(WidgetEvent::MOUSE_PRESS, WidgetDrawState::PRESSED);
        map_draw_state(WidgetEvent::MOUSE_RELEASE, WidgetDrawState::HOVERED);

        // Gestión de estados de interacción
        when_mouse_enter.connect([this](EventContext &) { m_is_hovered = true; });
        when_mouse_leave.connect([this](EventContext &) { m_is_hovered = false; });

        when_mouse_press.connect(
            [this](MouseButtonEventContext &ev)
            {
                auto now = std::chrono::steady_clock::now();
                auto duration =
                    std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_click_time)
                        .count();

                if (m_last_click_button == ev.button && duration < 200)
                {
                    if (m_click_timer)
                        application()->stop_timer(m_click_timer);

                    when_dbl_click.run(ev);
                    m_last_click_button = 0;
                    return;
                }

                m_last_click_time = now;
                m_last_click_button = ev.button;

                m_click_timer = application()->add_timer(200,
                                                         [this, &ev]()
                                                         {
                                                             when_click.run(ev);
                                                             m_last_click_button = 0;
                                                         });
            });

        when_mouse_release.connect([this](MouseButtonEventContext &ev) { m_is_pressed = false; });
    }

    Widget::~Widget()
    {
        if (m_app)
        {
            m_app->unregister_widget(this);
        }
    }

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

            count_non_free++;
        }

        if (count_non_free > 1)
        {
            m_free_space -= (m_spacing * (count_non_free - 1));
        }
    }

    int Widget::preferred_width() const
    {
        return m_fixed_size > 0 ? m_fixed_size : m_width;
    }

    int Widget::preferred_height() const
    {
        return m_fixed_size > 0 ? m_fixed_size : m_height;
    }

    int Widget::preferred_height(int /*width*/) const
    {
        return preferred_height();
    }

    void Widget::render(GraphicsContext &ctx, int cx, int cy, int cw, int ch, bool force)
    {
        if (!m_visible)
            return;

        // 1. Finalize layout before any rendering decisions
        calculate_layout();

        // 2. Check intersection with the dirty region using updated geometry
        bool intersects =
            !(m_x >= cx + cw || m_x + m_width <= cx || m_y >= cy + ch || m_y + m_height <= cy);

        if (!intersects)
        {
            return;
        }

        // 3. Determine if we need to draw based on finalized dirty state
        bool should_draw = m_dirty || force || m_child_dirty;

        if (should_draw)
        {
            draw(ctx);
        }

        int current_x = m_start_draw_x;
        int current_y = m_start_draw_y;

        int child_idx = 0;
        for (const auto &child : m_children)
        {
            if (child->position_type() == FREE)
            {
                child->render(ctx, cx, cy, cw, ch, should_draw);
                child_idx++;
                continue;
            }

            if (!child->is_visible())
            {
                child_idx++;
                continue;
            }

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
                int child_size =
                    (m_free_children_count > 0) ? (m_free_space / m_free_children_count) : 0;
                if (m_layout_type == WIDGET_LAYOUT_VERTICAL)
                {
                    child->set_position(current_x, current_y);
                    child->set_size(m_available_draw_width, child_size);
                    current_y += child_size + m_spacing;
                }
                else
                {
                    child->set_position(current_x, current_y);
                    child->set_size(child_size, m_available_draw_height);
                    current_x += child_size + m_spacing;
                }
            }

            child->render(ctx, cx, cy, cw, ch, should_draw);
            child_idx++;
        }

        m_dirty = false;
        m_child_dirty = false;
    }

    Widget *Widget::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled)
            return nullptr;

        if (x < m_x || y < m_y || x >= m_x + m_width || y >= m_y + m_height)
        {
            return nullptr;
        }

        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
        {
            Widget *child = it->get();
            if (Widget *hit = child->hit_test(x, y))
                return hit;
        }

        return this;
    }

    void Widget::add_child(std::unique_ptr<Widget> child)
    {
        if (!child)
            return;

        child->m_parent = this;
        child->set_application_recursive(m_app);
        child->set_window_recursive(m_window);
        m_children.push_back(std::move(child));
    }

    void Widget::add_child_at(int index, std::unique_ptr<Widget> child)
    {
        if (!child)
            return;

        child->m_parent = this;
        child->set_application_recursive(m_app);
        child->set_window_recursive(m_window);
        m_children.insert(m_children.begin() + index, std::move(child));
    }

    void Widget::clear_children()
    {
        m_children.clear();
        invalidate();
    }

    void Widget::remove_child(Widget *child)
    {
        for (auto it = m_children.begin(); it != m_children.end(); ++it)
        {
            if (it->get() == child)
            {
                m_children.erase(it);
                break;
            }
        }
        invalidate();
    }

    void Widget::remove_child_at(int index)
    {
        if (index >= 0 && index < (int)m_children.size())
        {
            m_children.erase(m_children.begin() + index);
            invalidate();
        }
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

    Window *Widget::window() const
    {
        if (m_window)
            return m_window;
        if (m_parent)
            return m_parent->window();
        return nullptr;
    }

    const std::vector<std::unique_ptr<Widget>> &Widget::children() const
    {
        return m_children;
    }

    WidgetDrawState Widget::get_draw_state() const
    {
        return m_draw_state;
    }

    WidgetDrawState Widget::get_draw_state(WidgetEvent event) const
    {
        if (m_draw_state_map.find(event) == m_draw_state_map.end())
            return WidgetDrawState::NORMAL;
        return m_draw_state_map.at(event);
    }

    void Widget::set_draw_state(WidgetDrawState draw_state)
    {
        if (m_draw_state != draw_state)
        {
            m_draw_state = draw_state;
            invalidate();
        }
    }

    void Widget::map_draw_state(WidgetEvent event, WidgetDrawState draw_state)
    {
        m_draw_state_map[event] = draw_state;
    }

    void Widget::set_position(int x, int y)
    {
        if (m_x != x || m_y != y)
        {
            m_x = x;
            m_y = y;
        }
    }

    void Widget::set_size(int width, int height)
    {
        if (m_width != width || m_height != height)
        {
            m_width = width;
            m_height = height;
            invalidate();
        }
    }

    void Widget::set_fixed_size(int size)
    {
        if (m_fixed_size != size)
        {
            m_fixed_size = size;
            invalidate();
        }
    }

    void Widget::set_spacing(int spacing)
    {
        if (m_spacing != spacing)
        {
            m_spacing = spacing;
            invalidate();
        }
    }

    void Widget::set_margin(int margin)
    {
        if (m_margin != margin)
        {
            m_margin = margin;
            invalidate();
        }
    }

    void Widget::set_position_type(WidgetPositionTypes position_type)
    {
        if (m_position_type != position_type)
        {
            m_position_type = position_type;
            invalidate();
        }
    }

    void Widget::set_layout_type(WidgetLayoutTypes layout_type)
    {
        if (m_layout_type != layout_type)
        {
            m_layout_type = layout_type;
            invalidate();
        }
    }

    void Widget::set_accent_color(WidgetAccentColor accent_color)
    {
        if (m_accent_color == accent_color)
            return;
        m_accent_color = accent_color;
        invalidate();
    }

    void Widget::set_background_color(const Color &color)
    {
        if (m_background_color.r != color.r || m_background_color.g != color.g ||
            m_background_color.b != color.b || m_background_color.a != color.a)
        {
            m_background_color = color;
            invalidate();
        }
    }

    Color Widget::background_color() const
    {
        return m_background_color;
    }

    void Widget::set_border_radius(int radius)
    {
        if (m_border_radius != radius)
        {
            m_border_radius = radius;
            invalidate();
        }
    }

    int Widget::border_radius() const
    {
        return m_border_radius;
    }

    void Widget::set_border_width(int width)
    {
        if (m_border_width != width)
        {
            m_border_width = width;
            invalidate();
        }
    }

    int Widget::border_width() const
    {
        return m_border_width;
    }

    void Widget::set_border_color(const Color &color)
    {
        if (m_border_color.r != color.r || m_border_color.g != color.g ||
            m_border_color.b != color.b || m_border_color.a != color.a)
        {
            m_border_color = color;
            invalidate();
        }
    }

    Color Widget::border_color() const
    {
        return m_border_color;
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
        if (m_visible != visible)
        {
            m_visible = visible;
            invalidate();
        }
    }
    bool Widget::is_visible() const
    {
        return m_visible;
    }
    bool Widget::is_effectively_visible() const
    {
        if (!m_visible)
            return false;
        if (m_parent)
            return m_parent->is_effectively_visible();
        return true;
    }
    void Widget::set_enabled(bool enabled)
    {
        if (m_enabled != enabled)
        {
            m_enabled = enabled;
            invalidate();
        }
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
            EventContext ev;
            ev.sender = this;
            if (m_has_focus)
            {
                when_focus.run(ev);
            }
            else
            {
                when_blur.run(ev);
            }
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

    void Widget::draw(GraphicsContext &ctx)
    {
        if (m_name == "NotebookHeader" || m_name == "NotebookBody" || m_name == "MainTextarea")
        {
            LOG_INFO << "[DRAW] " << m_name << " at " << m_x << "," << m_y << " " << m_width << "x"
                     << m_height;
        }
        if (m_background_color.a > 0.001f)
        {
            ctx.setColor(m_background_color);
            ctx.fillRect(m_x, m_y, m_width, m_height, m_border_radius);
        }

        if (m_border_width > 0 && m_border_color.a > 0.001f)
        {
            ctx.setColor(m_border_color);
            ctx.drawRect(m_x, m_y, m_width, m_height, m_border_radius, (float)m_border_width);
        }
    }

    void Widget::set_application_recursive(Application *app)
    {
        m_app = app;
        for (auto &child : m_children)
        {
            child->set_application_recursive(app);
        }
    }

    void Widget::set_window_recursive(Window *window)
    {
        m_window = window;
        for (auto &child : m_children)
        {
            child->set_window_recursive(window);
        }
    }

    void Widget::invalidate(Widget *widget)
    {
        m_dirty = true;
        Widget *p = m_parent;
        while (p)
        {
            if (p->m_child_dirty)
                break;
            p->m_child_dirty = true;
            p = p->m_parent;
        }

        if (window())
        {
            window()->invalidate(widget ? widget : this);
        }
    }

    size_t Widget::add_on_mouse_leave(std::function<void()> handler)
    {
        return when_mouse_leave.connect([handler](EventContext &) { handler(); });
    }
    void Widget::remove_on_mouse_leave(size_t id)
    {
        when_mouse_leave.disconnect(id);
    }

    size_t Widget::add_on_mouse_move(std::function<void(int, int)> handler)
    {
        return when_mouse_move.connect([handler](MouseMoveEventContext &ev)
                                       { handler((int)ev.x, (int)ev.y); });
    }
    void Widget::remove_on_mouse_move(size_t id)
    {
        when_mouse_move.disconnect(id);
    }

    size_t Widget::add_on_mouse_press(std::function<void(int)> handler)
    {
        return when_mouse_press.connect([handler](MouseButtonEventContext &ev)
                                        { handler(ev.button); });
    }
    void Widget::remove_on_mouse_press(size_t id)
    {
        when_mouse_press.disconnect(id);
    }

    size_t Widget::add_on_mouse_release(std::function<void(int)> handler)
    {
        return when_mouse_release.connect([handler](MouseButtonEventContext &ev)
                                          { handler(ev.button); });
    }
    void Widget::remove_on_mouse_release(size_t id)
    {
        when_mouse_release.disconnect(id);
    }

    size_t Widget::add_on_mouse_drag(std::function<void(int, int)> handler)
    {
        return when_mouse_drag.connect([handler](MouseMoveEventContext &ev)
                                       { handler((int)ev.x, (int)ev.y); });
    }
    void Widget::remove_on_mouse_drag(size_t id)
    {
        when_mouse_drag.disconnect(id);
    }

    size_t Widget::add_on_mouse_hover(std::function<void(int, int)> handler)
    {
        return when_mouse_hover.connect([handler](MouseMoveEventContext &ev)
                                        { handler((int)ev.x, (int)ev.y); });
    }
    void Widget::remove_on_mouse_hover(size_t id)
    {
        when_mouse_hover.disconnect(id);
    }

} // namespace horizon
