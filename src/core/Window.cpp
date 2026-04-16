#include "horizon/Application.hpp"
#include "horizon/Widget.hpp"
#include <horizon/GraphicsContext.hpp>
#include <horizon/Window.hpp>

namespace horizon
{

    Window::Window(std::string title)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto titlebar = std::make_unique<Titlebar>(title);

        titlebar->set_spacing(1);
        titlebar->set_spacing(8);
        titlebar->set_position(0, 0);
        titlebar->set_fixed_size(34);

        m_titlebar = titlebar.get(); // guardas puntero no propietario

        add_child(std::move(titlebar));
    }

    Window::Window(std::unique_ptr<Titlebar> custom_titlebar)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        m_titlebar = custom_titlebar.get();
        add_child(std::move(custom_titlebar));
    }

    void Window::set_size(int width, int height)
    {
        m_width = width;
        m_height = height;
        m_titlebar->set_size(m_width, 20);
    }

    const std::string &Window::title() const
    {
        return m_titlebar->title();
    }

    void Window::set_title(std::string title)
    {
        if (m_titlebar)
            m_titlebar->set_title(title);
        invalidate();
    }

    CornerRadius Window::get_window_corners() const
    {
        return {10, 10, 0, 0};
    }

    void Window::render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force)
    {
        Widget::render(gc, cx, cy, cw, ch, force);
    }

    void Window::draw(GraphicsContext &gc)
    {
        CornerRadius corners = get_window_corners();

        Color bg = application()->theme_manager->get_color("window_bg");
        Color bg2 = bg.lighter(10.0f);
        Color brd = application()->theme_manager->get_color("window_border");

        if (m_draw_background)
        {
            gc.setColor(bg);
            gc.fillRect(0, 0, m_width, m_height, corners);
        }

        gc.setColor(brd);
        gc.drawRect(0, 0, m_width, m_height, corners, 0.9f);

        if (m_draw_background)
        {
            // Dibujamos rectangulos horizontales oscuros cada 10 pixeles
            int titlebar_height = 30;
            int pattern_size = 4;
            int pattern_count = (m_height - titlebar_height) / (pattern_size * 2);
            int pattern_y = titlebar_height;

            for (int i = 0; i < pattern_count; i++)
            {
                gc.setColor(bg2);
                gc.fillRect(2, pattern_y, m_width - 4, pattern_size);
                pattern_y += pattern_size * 2;
            }
        }

        gc.flush();
    }

} // namespace horizon