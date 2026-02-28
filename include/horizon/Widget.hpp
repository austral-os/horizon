#pragma once

#include "horizon/EventsManager.hpp"
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

        friend class Application;

        // --- Geometría ---
        void set_position(int x, int y);
        void set_size(int width, int height);
        void set_fixed_size(int size);
        void set_spacing(int spacing);
        void set_margin(int margin);
        void set_position_type(WidgetPositionTypes position_type);
        void set_layout_type(WidgetLayoutTypes layout_type);
        void set_accent_color(WidgetAccentColor accent_color);

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

        // --- Estado ---
        void set_visible(bool visible);
        bool is_visible() const;

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
        Widget *parent() const;
        Application *application() const;

        const std::vector<std::unique_ptr<Widget>> &children() const;

        // --- Render ---
        virtual void render(GraphicsContext &ctx);

        /**
         * @brief Invalidates this widget, requesting a selective repaint.
         */
        void invalidate();

        EventsManager when_mouse_press;
        EventsManager when_mouse_enter;
        EventsManager when_mouse_leave;
        EventsManager when_mouse_move;
        EventsManager when_mouse_release;
        EventsManager when_mouse_drag;
        EventsManager when_mouse_hover;
        EventsManager when_key_press;
        EventsManager when_key_release;

        virtual void set_application_recursive(Application *app);

        void map_draw_state(WidgetEvent event, WidgetDrawState draw_state);

    protected:
        virtual void draw(GraphicsContext &ctx);

        Widget *hit_test(int x, int y);
        WidgetDrawState get_draw_state() const;
        WidgetDrawState get_draw_state(WidgetEvent event) const;

        void calculate_layout();
        void set_draw_state(WidgetDrawState draw_state);

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

        /*
         * m_widget_draw_state indica al metodo draw como debe dibujar el widget.
         * Esta propiedad puede ser especificada por parametro y no depender de los eventos.
         * Por defecto es NORMAL.
         * Si se establece en HOVERED, el widget se dibujara como si estuviera hovereado.
         * Si se establece en PRESSED, el widget se dibujara como si estuviera presionado.
         * Si se establece en DISABLED, el widget se dibujara como si estuviera deshabilitado.
         * Si se establece en FOCUSED, el widget se dibujara como si estuviera enfocado.
         * todo esto sin depender de los eventos.
         * Quizas podriamos especificar, opcionalmente en cada widget, en cual de estos estados
         * debe dibujarse segun el evento que ocurra. Seria mapear eventos a formas de dibujarse.
         */
        std::map<WidgetEvent, WidgetDrawState> m_draw_state_map;
        WidgetDrawState m_draw_state{WidgetDrawState::NORMAL};

        bool m_visible{true};
        bool m_enabled{true};
        bool m_focusable{false};
        bool m_has_focus{false};
        bool m_is_hovered{false};
        bool m_is_pressed{false};
        CursorType m_cursor_type{CursorType::Default};

        int m_start_draw_x{0};          // Posicion inicial x donde se puede comenzar a dibujar.
        int m_start_draw_y{0};          // Posicion inicial y donde se puede comenzar a dibujar.
        int m_available_draw_width{0};  // Ancho disponible para dibujar.
        int m_available_draw_height{0}; // Alto disponible para dibujar.
        int m_free_space{0};          // Espacio disponible para los hijos que no tienen fixed_size.
        int m_free_children_count{0}; // Cantidad de hijos que no tienen fixed_size.

        Widget *m_parent{nullptr};
        Application *m_app{nullptr};
        std::vector<std::unique_ptr<Widget>> m_children;

        size_t m_next_handler_id{0};
    };

} // namespace horizon