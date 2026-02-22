#pragma once

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
        Help
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
    };

} // namespace horizon