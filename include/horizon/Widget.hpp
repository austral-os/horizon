#pragma once

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
        void set_padding(int padding);
        void set_margin(int margin);
        void set_position_type(WidgetPositionTypes position_type);
        void set_layout_type(WidgetLayoutTypes layout_type);

        int x() const;
        int y() const;
        int width() const;
        int height() const;
        int fixed_size() const;
        int padding() const;
        int margin() const;
        WidgetPositionTypes position_type() const;
        WidgetLayoutTypes layout_type() const;

        // --- Estado ---
        void set_visible(bool visible);
        bool is_visible() const;

        void set_enabled(bool enabled);
        bool is_enabled() const;

        void set_focusable(bool focusable);
        bool is_focusable() const;

        bool has_focus() const;

        // --- Cursors ---
        void set_cursor_type(CursorType type);
        CursorType cursor_type() const;

        // --- Events (Universal Multi-Callback) ---

        // Mouse Enter
        size_t add_on_mouse_enter(std::function<void()> handler);
        void remove_on_mouse_enter(size_t id);

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

        // Key Press
        size_t add_on_key_press(std::function<void(int key)> handler);
        void remove_on_key_press(size_t id);

        // Key Release
        size_t add_on_key_release(std::function<void(int key)> handler);
        void remove_on_key_release(size_t id);

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
        void add_child(std::unique_ptr<Widget> child);
        Widget *parent() const;
        Application *application() const;

        const std::vector<std::unique_ptr<Widget>> &children() const;

        // --- Render ---
        virtual void render(GraphicsContext &ctx);

    protected:
        virtual void draw(GraphicsContext &ctx);

        // --- Eventos ---
        virtual void on_mouse_enter();
        virtual void on_mouse_leave();
        virtual void on_mouse_move(int x, int y);
        virtual void on_mouse_press(int button);
        virtual void on_mouse_release(int button);
        virtual void on_mouse_drag(int x, int y);
        virtual void on_mouse_hover(int x, int y);
        virtual void on_key_press(int key);
        virtual void on_key_release(int key);

        Widget *hit_test(int x, int y);

        void calculate_layout();

    protected:
        int m_x{0};
        int m_y{0};
        int m_width{0};
        int m_height{0};
        int m_fixed_size{-1};
        int m_padding{0};
        int m_margin{0};
        WidgetPositionTypes m_position_type{FREE};
        WidgetLayoutTypes m_layout_type{WIDGET_LAYOUT_HORIZONTAL};

        bool m_visible{true};
        bool m_enabled{true};
        bool m_focusable{false};
        bool m_has_focus{false};
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

        // Handler maps
        std::map<size_t, std::function<void()>> m_on_mouse_enter_handlers;
        std::map<size_t, std::function<void()>> m_on_mouse_leave_handlers;
        std::map<size_t, std::function<void(int, int)>> m_on_mouse_move_handlers;
        std::map<size_t, std::function<void(int)>> m_on_mouse_press_handlers;
        std::map<size_t, std::function<void(int)>> m_on_mouse_release_handlers;
        std::map<size_t, std::function<void(int, int)>> m_on_mouse_drag_handlers;
        std::map<size_t, std::function<void(int, int)>> m_on_mouse_hover_handlers;
        std::map<size_t, std::function<void(int)>> m_on_key_press_handlers;
        std::map<size_t, std::function<void(int)>> m_on_key_release_handlers;

        size_t m_next_handler_id{0};
    };

} // namespace horizon