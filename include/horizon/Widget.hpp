#pragma once

#include "horizon/Color.hpp"
#include "horizon/EventsManager.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace horizon
{

    class Application;
    class GraphicsContext;

    enum WidgetPositionTypes
    {
        FILL,
        FREE
    };

    enum WidgetLayoutTypes
    {
        WIDGET_LAYOUT_HORIZONTAL,
        WIDGET_LAYOUT_VERTICAL
    };

    enum class CursorType
    {
        Default,
        Pointer,
        Text,
        Move,
        Wait,
        Help,
        ResizeNS,
        ResizeEW,
        ResizeNESW,
        ResizeNWSE
    };

    enum class WidgetAccentColor
    {
        Default,
        Primary,
        Secondary,
        Success,
        Warning,
        Error,
        Info
    };

    /*
     * Eventos que puede disparar un widget.
     * Se utilizan para mapear eventos a formas de dibujarse.
     */
    enum class WidgetEvent
    {
        MOUSE_ENTER,
        MOUSE_LEAVE,
        MOUSE_MOVE,
        MOUSE_PRESS,
        MOUSE_RELEASE,
        MOUSE_DRAG,
        MOUSE_HOVER,
        KEY_PRESS,
        KEY_RELEASE
    };

    /*
     * Distintos estilos en los que puede dibujarse un widget.
     */
    enum class WidgetDrawState
    {
        NORMAL,
        HOVERED,
        PRESSED,
        FOCUSED,
        DISABLED,
        CUSTOM_1,
        CUSTOM_2,
        CUSTOM_3,
        CUSTOM_4,
        CUSTOM_5,
        CUSTOM_6,
        CUSTOM_7,
        CUSTOM_8,
        CUSTOM_9,
        CUSTOM_10
    };

    class Widget
    {
    public:
        Widget();
        virtual ~Widget();

        /**
         * @brief Performs a hit test to find the widget at the given coordinates.
         */
        virtual Widget *hit_test(int x, int y);

        friend class Application;

        // --- Geometría ---
        void set_position(int x, int y);
        virtual void set_size(int width, int height);
        void set_width(int width)
        {
            m_width = width;
        }
        void set_height(int height)
        {
            m_height = height;
        }
        void set_fixed_size(int size);
        void set_spacing(int spacing);
        void set_margin(int margin);
        void set_position_type(WidgetPositionTypes position_type);
        void set_layout_type(WidgetLayoutTypes layout_type);
        void set_accent_color(WidgetAccentColor accent_color);
        void set_background_color(const Color &color);
        void set_border_radius(int radius);
        void set_border_width(int width);
        void set_border_color(const Color &color);
        virtual void calculate_layout();

        virtual int preferred_width() const;
        virtual int preferred_height() const;
        virtual int preferred_height(int width) const;

        int x() const;
        int y() const;
        int width() const;
        int height() const;
        int fixed_size() const;
        int spacing() const;
        int margin() const;
        WidgetPositionTypes position_type() const;
        WidgetLayoutTypes layout_type() const;
        WidgetAccentColor accent_color() const;
        Color background_color() const;
        int border_radius() const;
        int border_width() const;
        Color border_color() const;
        
        void set_name(const std::string &name) { m_name = name; }
        std::string name() const { return m_name; }

        // --- Estado ---
        void set_visible(bool visible);
        bool is_visible() const;

        /**
         * @return True if the widget and all its parents are visible.
         */
        bool is_effectively_visible() const;

        void set_enabled(bool enabled);
        bool is_enabled() const;

        void set_focusable(bool focusable);
        bool is_focusable() const;

        void set_focus(bool focus);
        bool has_focus() const;

        bool is_hovered() const;
        bool is_pressed() const;

        // --- Cursors ---
        void set_cursor_type(CursorType type);
        CursorType cursor_type() const;

        // --- Events (Universal Multi-Callback) ---

        // Mouse Leave
        size_t add_on_mouse_leave(std::function<void()> handler);
        void remove_on_mouse_leave(size_t id);

        // Mouse Move
        size_t add_on_mouse_move(std::function<void(int x, int y)> handler);
        void remove_on_mouse_move(size_t id);

        // Mouse Press
        size_t add_on_mouse_press(std::function<void(int button)> handler);
        void remove_on_mouse_press(size_t id);

        // Mouse Release
        size_t add_on_mouse_release(std::function<void(int button)> handler);
        void remove_on_mouse_release(size_t id);

        // Mouse Drag
        size_t add_on_mouse_drag(std::function<void(int x, int y)> handler);
        void remove_on_mouse_drag(size_t id);

        // Mouse Hover
        size_t add_on_mouse_hover(std::function<void(int x, int y)> handler);
        void remove_on_mouse_hover(size_t id);

        // Backward compatibility for click
        size_t add_on_click(std::function<void()> handler)
        {
            return add_on_mouse_press(
                [handler](int btn)
                {
                    if (btn == 0x110)
                        handler();
                });
        }
        void set_on_click(std::function<void()> handler);

        // --- Árbol ---
        virtual void add_child(std::unique_ptr<Widget> child);
        virtual void add_child_at(int index, std::unique_ptr<Widget> child);
        void remove_child(Widget *child);
        void remove_child_at(int index);
        Widget *parent() const;
        Application *application() const;
        class Window *window() const;

        const std::vector<std::unique_ptr<Widget>> &children() const;
        void clear_children();

        // --- Render ---
        virtual void render(GraphicsContext &ctx, int cx, int cy, int cw, int ch,
                            bool force = false);

        /**
         * @brief Invalidates this widget, requesting a selective repaint.
         */
        virtual void invalidate(Widget *widget = nullptr);

        bool is_dirty() const
        {
            return m_dirty;
        }
        bool is_child_dirty() const
        {
            return m_child_dirty;
        }

        EventsManager<MouseButtonEventContext> when_mouse_press;
        EventsManager<MouseButtonEventContext> when_dbl_click;
        EventsManager<EventContext> when_mouse_enter;
        EventsManager<EventContext> when_mouse_leave;
        EventsManager<MouseMoveEventContext> when_mouse_move;
        EventsManager<MouseButtonEventContext> when_mouse_release;
        EventsManager<MouseWheelEventContext> when_mouse_wheel;
        EventsManager<MouseMoveEventContext> when_mouse_drag;
        EventsManager<MouseMoveEventContext> when_mouse_hover;
        EventsManager<KeyEventContext> when_key_press;
        EventsManager<KeyEventContext> when_key_release;
        EventsManager<EventContext> when_focus;
        EventsManager<EventContext> when_blur;

        virtual void set_application_recursive(Application *app);
        virtual void set_window_recursive(class Window *window);

        void map_draw_state(WidgetEvent event, WidgetDrawState draw_state);

        WidgetDrawState get_draw_state() const;
        WidgetDrawState get_draw_state(WidgetEvent event) const;

        void set_draw_state(WidgetDrawState draw_state);

    protected:
        virtual void draw(GraphicsContext &ctx);

    protected:
        int m_x{0};
        int m_y{0};
        int m_width{0};
        int m_height{0};
        int m_fixed_size{-1};
        int m_spacing{0};
        int m_margin{0};

        WidgetPositionTypes m_position_type{FREE};
        WidgetLayoutTypes m_layout_type{WIDGET_LAYOUT_HORIZONTAL};
        WidgetAccentColor m_accent_color{WidgetAccentColor::Default};
        Color m_background_color{0.0f, 0.0f, 0.0f, 0.0f}; // Default transparent
        int m_border_radius{0};
        int m_border_width{0};
        Color m_border_color{0.0f, 0.0f, 0.0f, 0.0f};

        std::map<WidgetEvent, WidgetDrawState> m_draw_state_map;
        WidgetDrawState m_draw_state{WidgetDrawState::NORMAL};

        bool m_visible{true};
        bool m_enabled{true};
        bool m_focusable{false};
        bool m_has_focus{false};
        bool m_is_hovered{false};
        bool m_is_pressed{false};
        CursorType m_cursor_type{CursorType::Default};

        int m_start_draw_x{0};
        int m_start_draw_y{0};
        int m_available_draw_width{0};
        int m_available_draw_height{0};
        int m_free_space{0};
        int m_free_children_count{0};

        Widget *m_parent{nullptr};
        Application *m_app{nullptr};
        class Window *m_window{nullptr};
        std::vector<std::unique_ptr<Widget>> m_children;

        size_t m_next_handler_id{0};
        bool m_dirty{true};
        bool m_child_dirty{true};

        std::string m_name;
        std::chrono::steady_clock::time_point m_last_click_time;
        uint32_t m_last_click_button{0};
    };

} // namespace horizon