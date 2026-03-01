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

    void Window::render(GraphicsContext &gc, bool force)
    {
        Widget::render(gc, force);
    }

    void Window::draw(GraphicsContext &gc)
    {

        Color bg = application()->theme_manager->get_color("window_bg");
        Color bg2 = bg.lighter(10.0f);
        Color brd = application()->theme_manager->get_color("window_border");

        gc.setColor(bg);
        gc.fillRect(m_start_draw_x, m_start_draw_y, m_available_draw_width, m_available_draw_height,
                    {10, 10, 0, 0});

        gc.setColor(brd);
        gc.drawRect(0, 0, m_width, m_height, {10, 10, 0, 0}, 0.9f);

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

        gc.flush();
    }

} // namespace horizon