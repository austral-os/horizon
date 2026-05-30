#pragma once
#include "horizon/ClipboardActions.hpp"
#include "horizon/ClipboardProvider.hpp"
#include "horizon/Color.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/ThemeManager.hpp"
#include <chrono>
#include <map>
#include <memory>
#include <vector>
#include <horizon/print/Models.h>

namespace horizon
{

    class Application;
    class WaylandWindow;
    class GraphicsContext;
    class Menu;
    class Vault;
    class Notification;

    /**
     * @brief Text alignment options for widgets.
     */
    enum class TextAlignment
    {
        Left,
        Center,
        Right
    };

    enum class WidgetOrientation
    {
        Horizontal,
        Vertical
    };

    /**
     * @brief Vertical alignment options for widgets.
     */
    enum class VerticalAlignment
    {
        Top,
        Middle,
        Bottom
    };

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
        ResizeNWSE,
        DndCopy,
        DndMove,
        DndNone
    };

    enum class WidgetAccentColor
    {
        Default,
        Primary,
        Secondary,
        Success,
        Warning,
        Error,
        Info,
        Custom
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

    struct widget_position
    {
        int x;
        int y;
    };

    class Widget : public ClipboardProvider
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
        void set_size(int width, int height);
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
        widget_position get_absolute_position() const;

        // --- Estado ---
        void set_visible(bool visible);
        bool is_visible() const;

        /**
         * @return True if the widget and all its parents are visible.
         */
        bool is_effectively_visible() const;

        virtual void set_enabled(bool enabled);
        virtual bool is_enabled() const;

        void set_focusable(bool focusable);
        bool is_focusable() const;

        void set_focus(bool focus);
        bool has_focus() const;

        bool is_hovered() const;
        bool is_pressed() const;

        // --- Cursors ---
        void set_cursor_type(CursorType type);
        CursorType cursor_type() const;

        // --- Debug Mode ---
        void set_debug_mode(bool debug_mode);
        bool debug_mode() const;

        // --- Clipboard ---
        virtual bool supports_undo() const
        {
            return false;
        }
        virtual bool supports_zoom() const
        {
            return false;
        }
        virtual bool supports_fullscreen() const
        {
            return false;
        }
        virtual bool supports_printing() const
        {
            return false;
        }
        virtual horizon::print::PrintDocument generate_print_document(const horizon::print::PrintConfig& config)
        {
            return {};
        }
        virtual bool supports_clipboard() const
        {
            return false;
        }
        virtual bool can_perform(ClipboardAction action) const
        {
            return false;
        }
        virtual void perform(ClipboardAction action) {}
        virtual std::vector<std::string> accepted_mime_types() const
        {
            return {};
        }
        virtual std::vector<std::string> provided_mime_types() const
        {
            return {};
        }
        virtual ClipboardProvider *get_clipboard_provider()
        {
            return this;
        }

        /**
         * @brief Standard implementation for providing clipboard data.
         * Default handles text/plain if the widget has related content.
         */
        virtual void provide_clipboard_data(const std::string &mime, DataSink &sink) {}

        /**
         * @brief Called when clipboard data is received (Paste).
         */
        virtual void on_clipboard_data_received(const std::string &mime,
                                                const std::vector<uint8_t> &data)
        {
        }

        // --- Drag and Drop ---
        void set_draggable(bool draggable)
        {
            m_draggable = draggable;
        }
        bool is_draggable() const
        {
            return m_draggable;
        }

        void set_accept_drops(bool accept)
        {
            m_accept_drops = accept;
        }
        bool accept_drops() const
        {
            return m_accept_drops;
        }
        // --- Context Menu ---
        void set_context_menu(std::unique_ptr<Menu> menu);
        Menu *context_menu() const;
        void set_vault(std::unique_ptr<Vault> vault);
        Vault *vault() const;
        void set_tooltip(std::unique_ptr<Notification> tooltip);
        Notification *tooltip() const;

        // --- Árbol ---
        virtual void add_child(std::unique_ptr<Widget> child);
        virtual void add_child_at(int index, std::unique_ptr<Widget> child);
        void remove_child(Widget *child);
        void remove_child_at(int index);
        Widget *parent() const;
        WaylandWindow *application() const;

        const std::vector<std::unique_ptr<Widget>> &children() const;
        void clear_children();

        // --- Render ---
        virtual void render(GraphicsContext &ctx, int cx, int cy, int cw, int ch,
                            bool force = false);

        /**
         * @brief Invalidates this widget, requesting a selective repaint.
         */
        void invalidate();

        bool is_dirty() const
        {
            return m_dirty;
        }
        bool is_child_dirty() const
        {
            return m_child_dirty;
        }

        EventsManager<MouseButtonEventContext> when_mouse_press;
        EventsManager<MouseButtonEventContext> when_click;
        EventsManager<MouseButtonEventContext> when_dbl_click;
        EventsManager<MouseButtonEventContext> when_middle_click;
        EventsManager<MouseButtonEventContext> when_right_click;
        EventsManager<EventContext> when_mouse_enter;
        EventsManager<EventContext> when_mouse_leave;
        EventsManager<MouseMoveEventContext> when_mouse_move;
        EventsManager<MouseButtonEventContext> when_mouse_release;
        EventsManager<MouseWheelEventContext> when_mouse_wheel;
        EventsManager<MouseMoveEventContext> when_mouse_drag;
        EventsManager<MouseMoveEventContext> when_mouse_hover;
        EventsManager<DragEventContext> when_drag_start;
        EventsManager<DragEventContext> when_drag_enter;
        EventsManager<DragEventContext> when_drag_leave;
        EventsManager<DragEventContext> when_drag_over;
        EventsManager<DropEventContext> when_drop;
        EventsManager<KeyEventContext> when_key_press;
        EventsManager<KeyEventContext> when_key_release;
        EventsManager<EventContext> when_focus;
        EventsManager<EventContext> when_blur;
        EventsManager<EventContext> when_application_load;
        EventsManager<FullscreenEventContext> when_enter_fullscreen;
        EventsManager<FullscreenEventContext> when_leave_fullscreen;
        EventsManager<EventContext> when_undo;
        EventsManager<EventContext> when_redo;
        EventsManager<EventContext> when_zoom_in;
        EventsManager<EventContext> when_zoom_out;

        virtual void set_application_recursive(WaylandWindow *app);

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
        bool m_debug_mode{false};
        CursorType m_cursor_type{CursorType::Default};
        std::unique_ptr<Menu> m_context_menu;
        std::unique_ptr<Vault> m_vault;

        int m_start_draw_x{0};
        int m_start_draw_y{0};
        int m_available_draw_width{0};
        int m_available_draw_height{0};
        int m_free_space{0};
        int m_free_children_count{0};

        Widget *m_parent{nullptr};
        WaylandWindow *m_app{nullptr};
        std::vector<std::unique_ptr<Widget>> m_children;

        size_t m_next_handler_id{0};
        bool m_dirty{true};
        bool m_child_dirty{true};

        std::chrono::steady_clock::time_point m_last_click_time;
        uint32_t m_last_click_button{0};
        uint32_t m_pressed_button{0};
        bool m_draggable{false};
        bool m_accept_drops{false};
        std::unique_ptr<Notification> m_tooltip;
        size_t m_tooltip_timer_id{0};

    private:
        static int get_double_click_speed();
    };

} // namespace horizon