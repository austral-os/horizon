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

        // Outer border (gray)
        Color borderColor(0.4f, 0.4f, 0.4f, 1.0f);
        gc.setColor(borderColor);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height,
                    {radius, radius, radius, radius}, 1.0f);

        // Top half: gradient from white to very light gray (creates a glass reflection effect)
        Color top1(0.85f, 0.85f, 0.85f, 1.0f);
        Color top2(0.8f, 0.8f, 0.8f, 1.0f);
        int halfHeight = m_height / 2;
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2, halfHeight,
                                  top1, top2, true, {radius - 1, radius - 1, 0, 0});

        // Bottom half: gradient starting slightly darker and lightening towards the bottom
        Color bot1(0.8f, 0.8f, 0.8f, 1.0f);
        Color bot2(0.97f, 0.97f, 0.97f, 1.0f);
        gc.fillLinearGradientRect(m_start_draw_x + 1, m_start_draw_y + 1 + halfHeight, m_width - 2,
                                  m_height - 2 - halfHeight, bot1, bot2, true,
                                  {0, 0, radius - 1, radius - 1});

        // Center the text
        TextMetrics metrics = gc.getTextMetrics(m_text.c_str(), "Lucida Grande", 18,
                                                FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);

        int text_x = m_start_draw_x + (m_width / 2) - (metrics.width / 2);
        int text_y = m_start_draw_y + (m_height / 2) + (metrics.height / 2) - 1;

        // Draw the text
        gc.setDrawFont("Lucida Grande", 18, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
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