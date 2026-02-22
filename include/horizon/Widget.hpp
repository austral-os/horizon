#pragma once

#include <memory>
#include <vector>

namespace horizon
{

    class Application;
    class GraphicsContext;

    class Widget
    {
    public:
        Widget();
        virtual ~Widget();

        friend class Application;

        // --- Geometría ---
        void set_position(int x, int y);
        void set_size(int width, int height);

        int x() const;
        int y() const;
        int width() const;
        int height() const;

        // --- Estado ---
        void set_visible(bool visible);
        bool is_visible() const;

        void set_enabled(bool enabled);
        bool is_enabled() const;

        void set_focusable(bool focusable);
        bool is_focusable() const;

        bool has_focus() const;

        // --- Árbol ---
        void add_child(std::unique_ptr<Widget> child);
        Widget *parent() const;

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

    protected:
        int m_x{0};
        int m_y{0};
        int m_width{0};
        int m_height{0};

    private:
        bool m_visible{true};
        bool m_enabled{true};
        bool m_focusable{false};
        bool m_has_focus{false};

        Widget *m_parent{nullptr};
        std::vector<std::unique_ptr<Widget>> m_children;
    };

} // namespace horizon