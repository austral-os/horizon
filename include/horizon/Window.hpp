#pragma once

#include "Widget.hpp"
#include "horizon/Titlebar.hpp"
#include <horizon/GraphicsContext.hpp>
#include <horizon/WaylandEventListener.hpp>
#include <horizon/Application.hpp>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <GLES2/gl2.h>
#include "horizon/CompositorContext.hpp"

namespace horizon
{
    class WaylandSurface;
    class Application;
    class LabwcCompositorContext;
    class WayfireCompositorContext;

    class Window : public Widget, public WaylandEventListener
    {
        friend class Application;
    public:
        Window(Application* app, std::string title, int w = 800, int h = 600);
        Window(Application* app, std::unique_ptr<Titlebar> custom_titlebar);
        virtual ~Window();

        void set_root(std::unique_ptr<Widget> root);
        void set_size(int width, int height) override;
        
        const std::string &title() const;
        virtual CornerRadius get_window_corners() const;

        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force = false) override;
        void draw(GraphicsContext &gc) override;

        // WaylandEventListener implementation
        void on_pointer_event(const PointerEvent &event) override;
        void on_key_event(const KeyEvent &event) override;
        void on_modifiers_event(uint32_t modifiers) override;
        void on_resize(int width, int height) override;
        void on_activated(bool active) override;
        void on_foreign_toplevel_event() override;
        void on_close() override;

        void invalidate(Widget *widget = nullptr) override;

        WaylandSurface* w_surface() { return m_surface.get(); }
        Application* app() { return m_app; }

        void request_move();
        void maximize();
        void minimize();
        void restore(const std::string &token = "");
        bool is_maximized() const;
        bool is_minimized() const { return m_is_minimized; }
        bool was_maximized_before_minimize() const { return m_was_maximized_before_minimize; }
        void fullscreen();
        void unfullscreen();
        bool is_fullscreen() const;

        void set_parent(Window* parent);
        Window* parent_window() const { return m_parent_window; }

        bool is_active() const { return m_is_active; }

        // Rendering bridge for CairoGraphicsContext
        void queue_gl_draw(const Application::GLDrawCall &call) {
            m_gl_queue.push_back(call);
        }

    protected:
        void handle_move(const PointerEvent &event);
        void handle_press(const PointerEvent &event);
        void handle_release(const PointerEvent &event);
        void handle_scroll(const PointerEvent &event);
        void handle_enter(const PointerEvent &event);
        void handle_leave(const PointerEvent &event);
        void handle_key_press(const KeyEvent &event);
        void handle_key_release(const KeyEvent &event);

        std::string m_title;
        std::unique_ptr<WaylandSurface> m_surface;
        std::unique_ptr<Widget> m_root;
        Titlebar* m_titlebar = nullptr;
        
        // Input state per window
        Widget *m_hovered = nullptr;
        Widget *m_pressed = nullptr;
        Widget *m_focused = nullptr;
        uint32_t m_modifiers = 0;
        double m_pointer_x = 0;
        double m_pointer_y = 0;
        uint32_t m_last_serial = 0;

        bool m_is_active = false;
        bool m_is_minimized = false;
        bool m_is_maximized = false;
        bool m_was_maximized_before_minimize = false;
        bool m_is_fullscreen = false;
        Window* m_parent_window = nullptr;

        // Rendering queue for this window's surface
        std::deque<Application::GLDrawCall> m_gl_queue;
        GLuint m_main_texture = 0;

        // Repaint tracking
        bool m_full_repaint = true;
        std::vector<Widget*> m_dirty_widgets;
    };
}