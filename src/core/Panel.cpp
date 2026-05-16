#include <horizon/Application.hpp>
#include <horizon/Panel.hpp>

namespace horizon
{
    Panel::Panel()
        : m_bg1(0.9f, 0.9f, 0.9f), m_bg2(0.8f, 0.8f, 0.8f), m_border_color(0.7f, 0.7f, 0.7f),
          m_corner_radius(0)
    {
    }

    void Panel::set_background_colors(Color bg1, Color bg2)
    {
        m_bg1 = bg1;
        m_bg2 = bg2;
    }

    void Panel::set_border_color(Color border)
    {
        m_border_color = border;
    }

    void Panel::set_corner_radius(CornerRadius radius)
    {
        m_corner_radius = radius;
    }
    
    void Panel::set_bottom_alpha(float alpha)
    {
        m_bottom_alpha = alpha;
    }

    void Panel::draw(GraphicsContext &gc)
    {

        auto *tm = application()->theme_manager.get();

        m_bg1 = tm->get_color("panel_bg1");
        m_bg2 = tm->get_color("panel_bg2");
        m_border_color = tm->get_color("panel_border");

        m_bg1.a *= m_bottom_alpha;

        // Dibujamos un fondo con degradado y un borde inferior
        gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_available_draw_width,
                                  m_available_draw_height, m_bg2, m_bg1, true, m_corner_radius);

        gc.setColor(m_border_color);
        gc.drawRect(0, m_height, m_width, 0, 0, 0.8f);
    }
} // namespace horizon
