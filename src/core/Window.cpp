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
        // Dibujar fondo de ventana con un gradiente
        std::cout << "dibujando ventana: w:" << m_width << " h:" << m_height << std::endl;

        Color darkBlue = {0.0f, 0.0f, 0.0f, 1.0f};
        Color lightBlue = {0.3f, 0.7f, 0.9f, 1.0f};

        gc.fillLinearGradientRect(0, 0, m_width, m_height, darkBlue, lightBlue, true);

        gc.flush();
        std::cout << "terminando ventana" << std::endl;
    }

} // namespace horizon