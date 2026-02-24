#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    Button::Button() : Widget() {}

    void Button::draw(GraphicsContext &gc)
    {
        int radius = m_height / 2;

        // Borde exterior adicional (sombra inferior, color más claro que el negro)
        Color shadowColor(0.4f, 0.4f, 0.4f, 1.0f);
        gc.setColor(shadowColor);
        gc.drawRect(m_start_draw_x, m_start_draw_y - 1, m_width, m_height,
                    {radius, radius, radius, radius}, 1.0f);

        // Outer border (gray/black)
        Color borderColor(0.35f, 0.35f, 0.35f, 1.0f);
        gc.setColor(borderColor);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height - 3,
                    {radius, radius, radius, radius}, 1.5f);

        // Top half: gradient from white to very light gray (creates a glass reflection effect)
        Color top1(0.85f, 0.85f, 0.85f, 1.0f);
        Color top2(0.8f, 0.8f, 0.8f, 1.0f);
        int halfHeight = m_height / 2;
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2, halfHeight,
                                  top1, top2, true, {radius - 1, radius - 1, 0, 0});

        // Bottom half: gradient starting slightly darker and lightening towards the bottom
        Color bot1(0.8f, 0.8f, 0.8f, 1.0f);
        Color bot2(0.97f, 0.97f, 0.97f, 1.0f);
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + halfHeight, m_width - 2,
                                  m_height - 4 - halfHeight, bot1, bot2, true,
                                  {0, 0, radius - 1, radius - 1});

        // Inner top highlight (simulates strong light reflection on top of the glass)
        int h_margin_x = radius / 4; // Margen a los lados para que el brillo sea más chico
        int h_width = m_width - (h_margin_x * 2);
        int h_height = halfHeight - 2;       // Un poco más chico que la altura de la mitad superior
        int h_radius_top = radius - 2;       // El radio superior se adapta al botón
        int h_radius_bot = h_radius_top / 2; // El radio inferior es mucho más curvo/chico

        Color highlight(1.0f, 1.0f, 1.0f, 0.7f); // Blanco puro con un poco de transparencia
        gc.fillLinearGradientRect(m_start_draw_x + h_margin_x, m_start_draw_y + 2, h_width,
                                  h_height, Color(1.0f, 1.0f, 1.0f, 1.0f), // Blanco sólido arriba
                                  Color(1.0f, 1.0f, 1.0f, 0.5f), // Blanco casi transparente abajo
                                  true, {h_radius_top, h_radius_top, h_radius_bot, h_radius_bot});

        // Center the text
        TextMetrics metrics = gc.getTextMetrics(m_text.c_str(), "Lucida Grande", 20,
                                                FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        int text_x = m_start_draw_x + (m_width / 2) - (metrics.width / 2);
        int text_y = m_start_draw_y + (m_height / 2) + (metrics.height / 2) - 3;

        // Draw the text
        gc.setDrawFont("Lucida Grande", 20, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
        gc.setColor(Color(0.1f, 0.1f, 0.1f, 1.0f));
        gc.drawText(text_x, text_y, m_text.c_str());
    }

    void Button::set_text(std::string text)
    {
        m_text = std::move(text);
    }

    const std::string &Button::text() const
    {
        return m_text;
    }

} // namespace horizon