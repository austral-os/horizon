#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/WaylandSurface.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Logger.hpp>
#include <wayland-client.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/egl.h>

namespace horizon
{

    Window::Window(Application* app, std::string title, int w, int h, bool setup_toplevel)
        : m_title(title)
    {
        m_app = app;
        m_width = w;
        m_height = h;

        // Create surface
        m_surface = std::make_unique<WaylandSurface>(app, w, h);
        
        // Pass shared globals to surface
        m_surface->set_wl_display(app->wl_display());
        m_surface->set_wl_compositor(app->wl_compositor());
        m_surface->set_wl_shm(app->wl_shm());
        m_surface->set_xdg_wm_base(app->xdg_wm_base());
        m_surface->set_zwlr_layer_shell(app->wl_layer_shell());
        m_surface->set_wl_seat(app->wl_seat());
        m_surface->set_xdg_activation(app->xdg_activation());
        m_surface->set_zwlr_foreign_toplevel_manager(app->foreign_toplevel_manager());
        m_surface->set_ext_background_effect_manager(app->background_effect_manager());
        m_surface->set_ext_foreign_toplevel_list(app->ext_foreign_toplevel_list());
        m_surface->set_blur_manager(app->blur_manager());
        for (auto* out : app->outputs()) m_surface->add_wl_output(out);
        
        m_surface->set_egl_display(app->m_egl_display);
        m_surface->set_egl_config(app->m_egl_config);
        m_surface->set_egl_context(app->m_egl_context);
        
        if (setup_toplevel) {
            m_surface->setup_xdg_toplevel(title, app->app_id());
            
            auto titlebar = std::make_unique<Titlebar>(title);
            titlebar->set_fixed_size(34);
            m_titlebar = titlebar.get();
            add_child(std::move(titlebar));
        }
        
        m_surface->set_event_listener(this);

        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        
        set_window_recursive(this);
    }

    Window::Window(Application* app, std::unique_ptr<Titlebar> custom_titlebar)
    {
        m_app = app;
        m_width = 800; // Default size
        m_height = 600;
        m_title = custom_titlebar->title();

        // Create surface
        m_surface = std::make_unique<WaylandSurface>(app, m_width, m_height);
        
        // Pass shared globals to surface
        m_surface->set_wl_display(app->wl_display());
        m_surface->set_wl_compositor(app->wl_compositor());
        m_surface->set_wl_shm(app->wl_shm());
        m_surface->set_xdg_wm_base(app->xdg_wm_base());
        m_surface->set_zwlr_layer_shell(app->wl_layer_shell());
        m_surface->set_wl_seat(app->wl_seat());
        m_surface->set_xdg_activation(app->xdg_activation());
        m_surface->set_zwlr_foreign_toplevel_manager(app->foreign_toplevel_manager());
        m_surface->set_ext_background_effect_manager(app->background_effect_manager());
        m_surface->set_ext_foreign_toplevel_list(app->ext_foreign_toplevel_list());
        m_surface->set_blur_manager(app->blur_manager());
        for (auto* out : app->outputs()) m_surface->add_wl_output(out);
        
        m_surface->set_egl_display(app->m_egl_display);
        m_surface->set_egl_config(app->m_egl_config);
        m_surface->set_egl_context(app->m_egl_context);
        
        m_surface->setup_xdg_toplevel(m_title, app->app_id());
        m_surface->set_event_listener(this);

        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        m_titlebar = custom_titlebar.get();
        add_child(std::move(custom_titlebar));
        set_window_recursive(this);
    }

    Window::~Window()
    {
        if (m_app) {
            m_app->unregister_window(this);
        }
    }

    void Window::set_size(int width, int height)
    {
        this->Widget::set_size(width, height);
        if (m_surface && (m_surface->width() != width || m_surface->height() != height))
        {
            m_surface->resize_buffer(width, height);
        }
        if (m_titlebar) m_titlebar->set_size(m_width, m_titlebar->preferred_height());
        if (m_root) m_root->set_size(m_width, m_height);
    }

    const std::string &Window::title() const
    {
        return m_title;
    }

    CornerRadius Window::get_window_corners() const
    {
        return {10, 10, 0, 0};
    }

    void Window::render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force)
    {
        Widget::render(gc, cx, cy, cw, ch, force);
        m_full_repaint = false;
        m_dirty_widgets.clear();
    }

    void Window::draw(GraphicsContext &gc)
    {
        CornerRadius corners = get_window_corners();

        Color bg = m_app->theme_manager->get_color("window_bg");
        Color brd = m_app->theme_manager->get_color("window_border");

        // Use custom background if set, or if the surface needs to be transparent
        if (m_background_color.a > 0.0f || m_app->is_transparent_surface()) {
            bg = m_background_color;
        }

        bool is_transparent = m_app->is_transparent_surface() || m_background_color.a < 1.0f;

        if (is_transparent) {
            gc.clearRect(0, 0, m_width, m_height, corners);
        }

        if (bg.a > 0.0f) {
            gc.setColor(bg);
            gc.fillRect(0, 0, m_width, m_height, corners);
        }

        if (brd.a > 0.0f && !is_transparent) {
            gc.setColor(brd);
            gc.drawRect(0, 0, m_width, m_height, corners, 0.9f);
        }

        gc.flush();
    }

    // WaylandEventListener implementation
    void Window::on_pointer_event(const PointerEvent &event)
    {
        m_pointer_x = event.x;
        m_pointer_y = event.y;

        switch (event.type)
        {
        case PointerEvent::Type::Move:
            handle_move(event);
            break;
        case PointerEvent::Type::Press:
            handle_press(event);
            break;
        case PointerEvent::Type::Release:
            handle_release(event);
            break;
        case PointerEvent::Type::Scroll:
            handle_scroll(event);
            break;
        case PointerEvent::Type::Enter:
            handle_enter(event);
            break;
        case PointerEvent::Type::Leave:
            handle_leave(event);
            break;
        default:
            break;
        }
    }

    void Window::on_key_event(const KeyEvent &event)
    {
        if (event.type == KeyEvent::Type::Press) handle_key_press(event);
        else handle_key_release(event);
    }

    void Window::on_modifiers_event(uint32_t modifiers) { m_modifiers = modifiers; }
    
    void Window::on_resize(int width, int height)
    {
        set_size(width, height);
        m_full_repaint = true;
        invalidate();
    }

    void Window::on_activated(bool active)
    {
        m_is_active = active;
    }

    void Window::on_close()
    {
        LOG_INFO << "Window " << title() << " received close request";
        if (m_app) m_app->unregister_window(this);
    }

    void Window::on_foreign_toplevel_event()
    {
        if (m_app)
            m_app->on_foreign_toplevel_event();
    }


    void Window::handle_move(const PointerEvent &event)
    {
        Widget *target = m_root ? m_root.get() : this;
        Widget *under = target->hit_test(event.x, event.y);
        
        if (under != m_hovered) {
            Widget *old_hovered = m_hovered;
            Widget *new_hovered = under;

            // Find common ancestor
            std::vector<Widget*> old_path;
            Widget* curr = old_hovered;
            while (curr) {
                old_path.push_back(curr);
                curr = curr->parent();
            }

            std::vector<Widget*> new_path;
            curr = new_hovered;
            while (curr) {
                new_path.push_back(curr);
                curr = curr->parent();
            }

            Widget* common_ancestor = nullptr;
            auto it_old = old_path.rbegin();
            auto it_new = new_path.rbegin();
            while (it_old != old_path.rend() && it_new != new_path.rend() && *it_old == *it_new) {
                common_ancestor = *it_old;
                ++it_old;
                ++it_new;
            }

            // Leave old ones up to common ancestor
            for (Widget* w : old_path) {
                if (w == common_ancestor) break;
                EventContext ev;
                ev.sender = w;
                w->when_mouse_leave.run(ev);
            }

            m_hovered = new_hovered;

            // Enter new ones up to common ancestor
            // Path is from leaf to root, so we need to iterate in reverse to fire from parent to child
            // Wait, usually Enter is fired from parent to child, or child to parent?
            // In many toolkits it's from root to leaf. But here we just need them to fire.
            // Let's go from leaf to common ancestor for consistency with Leave.
            for (Widget* w : new_path) {
                if (w == common_ancestor) break;
                EventContext ev;
                ev.sender = w;
                w->when_mouse_enter.run(ev);
            }
        }

        if (m_pressed) {
            MouseMoveEventContext ev;
            ev.sender = m_pressed;
            ev.x = event.x;
            ev.y = event.y;
            ev.modifiers = m_modifiers;
            
            Widget* current = m_pressed;
            while (current) {
                current->when_mouse_drag.run(ev);
                if (ev.stop_propagation) break;
                current = current->parent();
            }
        } else if (m_hovered) {
            MouseMoveEventContext ev;
            ev.sender = m_hovered;
            ev.x = event.x;
            ev.y = event.y;
            ev.modifiers = m_modifiers;

            Widget* current = m_hovered;
            while (current) {
                current->when_mouse_move.run(ev);
                if (ev.stop_propagation) break;
                current = current->parent();
            }
        }
    }

    void Window::handle_press(const PointerEvent &event)
    {
        Widget *target = m_root ? m_root.get() : this;
        Widget *under = target->hit_test(event.x, event.y);
        
        if (under) {
            m_pressed = under;
            
            // Transfer focus to the nearest focusable parent (or the widget itself)
            Widget *focusable = under;
            while (focusable && !focusable->is_focusable()) {
                focusable = focusable->parent();
            }

            if (focusable && focusable->is_focusable()) {
                if (m_focused && m_focused != focusable) m_focused->set_focus(false);
                m_focused = focusable;
                m_focused->set_focus(true);
            }
            
            MouseButtonEventContext ev;
            ev.sender = under;
            ev.button = event.button;
            ev.modifiers = m_modifiers;
            ev.x = event.x;
            ev.y = event.y;
            
            Widget* current = under;
            while (current) {
                current->when_mouse_press.run(ev);
                if (ev.stop_propagation) break;
                current = current->parent();
            }
        }
    }

    void Window::handle_release(const PointerEvent &event)
    {
        if (m_pressed) {
            MouseButtonEventContext ev;
            ev.sender = m_pressed;
            ev.button = event.button;
            ev.modifiers = m_modifiers;
            ev.x = event.x;
            ev.y = event.y;
            
            Widget* current = m_pressed;
            while (current) {
                current->when_mouse_release.run(ev);
                if (ev.stop_propagation) break;
                current = current->parent();
            }
            m_pressed = nullptr;
        }
    }

    void Window::handle_scroll(const PointerEvent &event)
    {
        Widget *target = m_hovered ? m_hovered : (m_root ? m_root.get() : this);
        
        MouseWheelEventContext ev;
        ev.x = event.x;
        ev.y = event.y;
        ev.dx = event.dx;
        ev.dy = event.dy;

        // Bubble up if not handled
        Widget* current = target;
        while (current) {
            current->when_mouse_wheel.run(ev);
            if (ev.stop_propagation) break;
            current = current->parent();
        }
    }

    void Window::handle_enter(const PointerEvent &event)
    {
        handle_move(event);
    }

    void Window::handle_leave(const PointerEvent &event)
    {
        if (m_hovered) {
            EventContext ev;
            ev.sender = m_hovered;
            m_hovered->when_mouse_leave.run(ev);
            m_hovered = nullptr;
        }
    }

    void Window::handle_key_press(const KeyEvent &event)
    {
        Widget *target = m_focused ? m_focused : (m_root ? m_root.get() : this);
        
        KeyEventContext ev;
        ev.key = event.key;
        ev.keysym = event.keysym;
        ev.text = event.text;
        ev.modifiers = m_modifiers;
        
        Widget* current = target;
        while (current) {
            current->when_key_press.run(ev);
            if (ev.stop_propagation) break;
            current = current->parent();
        }

        if (!ev.stop_propagation && event.keysym == XKB_KEY_Escape)
        {
            LOG_INFO << "Escape key pressed, closing window";
            on_close();
        }
    }

    void Window::handle_key_release(const KeyEvent &event)
    {
        Widget *target = m_focused ? m_focused : (m_root ? m_root.get() : this);
        KeyEventContext ev;
        ev.key = event.key;
        ev.keysym = event.keysym;
        ev.text = event.text;
        ev.modifiers = m_modifiers;
        
        Widget* current = target;
        while (current) {
            current->when_key_release.run(ev);
            if (ev.stop_propagation) break;
            current = current->parent();
        }
    }

    void Window::set_root(std::unique_ptr<Widget> root)
    {
        m_root = std::move(root);
        m_root->set_size(m_width, m_height);
        m_root->set_window_recursive(this);
    }

    void Window::invalidate(Widget *widget)
    {
        m_full_repaint = true;
        if (widget) m_dirty_widgets.push_back(widget);
        if (m_app) m_app->wakeup();
    }

    void Window::request_move() { if (m_surface) m_surface->request_move(m_last_serial); }
    void Window::maximize() { if (m_surface) m_surface->request_maximize(); }
    void Window::minimize() { if (m_surface) m_surface->request_minimize(); }
    void Window::restore(const std::string &token) { if (m_surface) m_surface->request_restore(); }
    bool Window::is_maximized() const { return m_surface ? m_surface->is_maximized() : false; }
    void Window::fullscreen() { if (m_surface) m_surface->request_fullscreen(); }
    void Window::unfullscreen() { if (m_surface) m_surface->request_unfullscreen(); }
    bool Window::is_fullscreen() const { return m_surface ? m_surface->is_fullscreen() : false; }

    void Window::set_parent(Window* parent)
    {
        m_parent_window = parent;
    }

} // namespace horizon