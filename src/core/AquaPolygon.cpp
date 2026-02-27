#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/AquaPolygon.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    AquaPolygon::AquaPolygon() : AquaObject() {}

    void AquaPolygon::set_points(const std::vector<PolygonPoint> &points)
    {
        m_points = points;
        invalidate();
    }

    const std::vector<PolygonPoint> &AquaPolygon::points() const
    {
        return m_points;
    }

    void AquaPolygon::set_has_border(bool has_border)
    {
        m_has_border = has_border;
        invalidate();
    }

    bool AquaPolygon::has_border() const
    {
        return m_has_border;
    }

    void AquaPolygon::set_border_size(float size)
    {
        m_border_size = size;
        invalidate();
    }

    void AquaPolygon::draw(GraphicsContext &gc)
    {
        if (m_points.empty())
            return;

        auto *tm = application()->theme_manager.get();

        Color window_bg = tm->get_color("window_bg");
        Color c1 = tm->get_color("default1");
        Color c2 = tm->get_color("default2");
        Color window_fg = tm->get_color("window_fg");
        Color shadow_color = tm->get_color("window_border");
        Color border_color = shadow_color.darker(10.0f);

        switch (m_accent_color)
        {
        case WidgetAccentColor::Default:
            c1 = tm->get_color("default1");
            c2 = tm->get_color("default2");
            break;
        case WidgetAccentColor::Primary:
            c1 = tm->get_color("primary1");
            c2 = tm->get_color("primary2");
            break;
        case WidgetAccentColor::Secondary:
            c1 = tm->get_color("secondary1");
            c2 = tm->get_color("secondary2");
            break;
        case WidgetAccentColor::Success:
            c1 = tm->get_color("success1");
            c2 = tm->get_color("success2");
            break;
        case WidgetAccentColor::Warning:
            c1 = tm->get_color("warning1");
            c2 = tm->get_color("warning2");
            break;
        case WidgetAccentColor::Error:
            c1 = tm->get_color("error1");
            c2 = tm->get_color("error2");
            break;
        case WidgetAccentColor::Info:
            c1 = tm->get_color("info1");
            c2 = tm->get_color("info2");
            break;
        }

        Color top1 = c1;
        Color top2 = c2;
        Color bot1 = c2;
        Color bot2 = c2.lighter(30.0f);
        Color highlight = window_bg;
        Color highlight2 = highlight.with_alpha(0.3f);

        if (is_hovered())
        {
            highlight = highlight.lighter(100.0f);
            highlight2 = highlight.with_alpha(0.5f);
        }

        // Adjust points to widget position
        std::vector<PolygonPoint> absolute_points = m_points;
        int min_y = 1e9, max_y = -1e9;
        for (auto &p : absolute_points)
        {
            p.x += m_start_draw_x;
            p.y += m_start_draw_y;
            min_y = std::min(min_y, p.y);
            max_y = std::max(max_y, p.y);
        }

        int poly_height = max_y - min_y;
        int half_height = poly_height / 2;

        // 1. Draw background/shadow if border enabled
        if (m_has_border)
        {
            // Shadow
            gc.setColor(shadow_color);
            std::vector<PolygonPoint> shadow_points = absolute_points;
            for (auto &p : shadow_points)
                p.y += 2;
            gc.drawPolygon(shadow_points, 1.0f);

            // Outer border
            gc.setColor(border_color);
            gc.drawPolygon(absolute_points, m_border_size);
        }

        // 2. Fill background
        gc.setColor(window_bg);
        gc.fillPolygon(absolute_points);

        // 3. Aqua Gradient (Main effect)
        gc.fillLinearGradientPolygon(absolute_points, top1, bot2, true);
    }

} // namespace horizon
