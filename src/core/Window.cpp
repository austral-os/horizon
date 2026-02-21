#include <horizon/GraphicsContext.hpp>
#include <horizon/Window.hpp>
#include <iostream>

namespace horizon
{

    Window::Window(std::string title) : m_title(std::move(title)) {}

    void Window::setSize(int width, int height)
    {
        m_width = width;
        m_height = height;
    }

    const std::string &Window::title() const
    {
        return m_title;
    }

    void Window::render(GraphicsContext &gc)
    {
        Widget::render(gc);
    }

    void Window::draw(GraphicsContext &gc)
    {
        // Dibujar fondo de ventana
        std::cout << "dibujando ventana: w:" << m_width << " h:" << m_height << std::endl;
        gc.setColor(0.2, 0.6, 0.8, 1.0);
        gc.fillRect(0, 0, m_width, m_height);
        gc.flush();
        std::cout << "terminando ventana" << std::endl;
    }

} // namespace horizon