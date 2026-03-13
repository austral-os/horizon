#pragma once

#include "horizon/CompositorAppInterface.hpp"
#include "horizon/CompositorContext.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/ThemeManager.hpp"
#include "horizon/WaylandSurface.hpp"
#include "horizon/Widget.hpp"
#include <horizon/WaylandEventListener.hpp>
namespace horizon
{

    class GraphicsContext;

    class HznSurface : public WaylandEventListener
    {
        friend class CairoGraphicContext;

    public:
        HznSurface();
        virtual ~HznSurface();

        std::unique_ptr<ThemeManager> theme_manager;

        EventsManager<AppEventContext> when_activated;
        EventsManager<AppEventContext> when_deactivated;
        EventsManager<AppEventContext> when_close;
        EventsManager<AppListEventContext> when_foreign_update;

        /**
         * @brief Returns whether this application uses a transparent surface.
         * @return true for LayerApplication, false for regular applications.
         */
        virtual bool is_transparent_surface() const
        {
            return true;
        }

        /**
         * @brief Gets the underlying WaylandSurface.
         * @return Pointer to the WaylandSurface managed by this application.
         */
        WaylandSurface *w_surface() const;

        int width() const;
        int height() const;

        size_t add_timer(int ms, std::function<void()> callback, bool repeat = false);
        void stop_timer(size_t id);

        /**
         * @brief Unregisters a widget from internal application state (e.g. dirty lists, focus).
         * Called when a widget is destroyed.
         */
        void unregister_widget(Widget *widget);

        /**
         * @brief Sets the root widget of the application's widget tree.
         * @param root Unique pointer to the new root widget.
         */
        void set_root(std::unique_ptr<Widget> root);

        /**
         * @brief Invalidates the entire application or a specific widget.
         * @param widget The widget to invalidate. If nullptr, the entire window is repainted.
         */
        void invalidate(Widget *widget = nullptr);

        /**
         * @brief Signals the application to wake up its event loop (e.g. from another thread).
         */
        void wakeup();

        /**
         * @brief Returns the graphics context for this application.
         */
        virtual GraphicsContext &get_graphics_context() const;

        GLuint gl_program() const
        {
            return m_gl_program;
        }
        GLuint gl_vbo() const
        {
            return m_gl_vbo;
        }

        struct GLDrawCall
        {
            uint32_t texture_id;
            float mvp[16];
            float opacity;
            bool delete_texture;
            bool use_scissor;
            int scissor_x, scissor_y, scissor_w, scissor_h;
        };

        void queue_gl_draw(const GLDrawCall &call) const;

    protected:
        std::unique_ptr<WaylandSurface> m_surface;
        std::unique_ptr<Widget> m_root;
        bool m_full_repaint = true; /**< Flag indicating if the entire UI needs re-rendering. */
        std::vector<Widget *> m_dirty_widgets; /**< List of widgets that need re-rendering. */
        int m_wakeup_fd{-1};                   /**< File descriptor for waking up the event loop. */

        Widget *m_hovered = nullptr; /**< The widget currently under the mouse pointer. */
        Widget *m_pressed = nullptr; /**< The widget currently being pressed by a mouse button. */
        Widget *m_focused = nullptr; /**< The widget currently having keyboard focus. */

        struct Timer
        {
            size_t id;
            int interval_ms;
            uint64_t next_expiry;
            bool repeat;
            std::function<void()> callback;
        };
        std::map<size_t, Timer> m_timers;
        size_t m_next_timer_id{1};

        GLuint m_gl_program{0};
        GLuint m_gl_vbo{0};
        GLuint m_gl_texture{0};
        mutable std::vector<GLDrawCall> m_gl_queue;

        void init_gl_resources();
        void render_gl_ui();

        mutable std::unique_ptr<GraphicsContext> m_gc;
        std::unique_ptr<CompositorContext> m_compositor_context;

        // Image caching
        mutable std::map<std::string, void *> m_svg_cache;
        mutable std::map<std::string, void *> m_surface_cache;
    };

}; // namespace horizon