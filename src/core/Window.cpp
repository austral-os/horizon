#include "horizon/Widget.hpp"
#include <horizon/GraphicsContext.hpp>
#include <horizon/Window.hpp>
#include <iostream>

namespace horizon
{

    Window::Window(std::string title)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto titlebar = std::make_unique<Titlebar>(title);

        titlebar->set_padding(1);
        titlebar->set_position(0, 0);
        titlebar->set_fixed_size(30);

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

    void Window::render(GraphicsContext &gc)
    {
        Widget::render(gc);
    }

    void Window::draw(GraphicsContext &gc)
    {
        // Dibujar fondo de ventana con un gradiente
        std::cout << "dibujando ventana: w:" << m_width << " h:" << m_height << std::endl;

        Color darkBlue = {0.0f, 0.0f, 0.0f, 1.0f};
        Color lightBlue = {0.3f, 0.7f, 0.9f, 1.0f};
        Color brd = {0.47f, 0.47f, 0.47f, 1.0f};

        Color bg = {0.953f, 0.953f, 0.953f, 1.0f};

        gc.setColor(bg);
        gc.fillRect(m_start_draw_x, m_start_draw_y, m_available_draw_width, m_available_draw_height,
                    {10, 10, 0, 0});

        gc.setColor(brd);
        gc.drawRect(0, 0, m_width, m_height, {10, 10, 0, 0}, 0.9f);

        // gc.fillLinearGradientRect(0, 0, m_width, m_height, darkBlue, lightBlue, true, 20);

        gc.flush();
        std::cout << "terminando ventana" << std::endl;
    }

} // namespace horizon