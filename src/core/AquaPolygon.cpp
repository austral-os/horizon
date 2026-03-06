#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/AquaPolygon.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    AquaPolygon::AquaPolygon() : AquaObject() {}

    void AquaPolygon::set_points(const std::vector<PolygonPoint> &points)
    {
        if (m_points.size() == points.size())
        {
            bool changed = false;
            for (size_t i = 0; i < points.size(); ++i)
            {
                if (m_points[i].x != points[i].x || m_points[i].y != points[i].y ||
                    m_points[i].radius != points[i].radius)
                {
                    changed = true;
                    break;
                }
            }
            if (!changed)
                return;
        }

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
        int min_x = 1e9, max_x = -1e9;
        int min_y = 1e9, max_y = -1e9;
        for (auto &p : absolute_points)
        {
            p.x += m_start_draw_x;
            p.y += m_start_draw_y;
            min_x = std::min(min_x, p.x);
            max_x = std::max(max_x, p.x);
            min_y = std::min(min_y, p.y);
            max_y = std::max(max_y, p.y);
        }

        int poly_width = max_x - min_x;
        int poly_height = max_y - min_y;
        int half_height = poly_height / 2;
        int center_x = (min_x + max_x) / 2;
        int center_y = (min_y + max_y) / 2;

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

        // 2. Fill background (clears the shape)
        gc.setColor(window_bg);
        gc.fillPolygon(absolute_points);

        // 3. Aqua Style Gradients (Top and Bottom halves)
        gc.save();
        gc.clipPolygon(absolute_points);

        // --- Top half: top1 -> top2 ---
        // We draw a gradient rectangle exactly on the top half.
        // Because we are clipped to the polygon, it only shows inside.
        gc.fillLinearGradientRect(min_x, min_y, poly_width, half_height, top1, top2, true, 0);

        // --- Bottom half: bot1 -> bot2 ---
        // We draw a gradient rectangle exactly on the bottom half.
        int rem_height = poly_height - half_height;
        gc.fillLinearGradientRect(min_x, min_y + half_height, poly_width, rem_height, bot1, bot2,
                                  true, 0);

        gc.restore();

        // 4. Inner top highlight (The "Glass" reflection)
        // We shrink the polygon slightly to create an inner reflection area.
        std::vector<PolygonPoint> inner_highlight_points;
        for (const auto &p : absolute_points)
        {
            int nx = p.x;
            int ny = p.y;

            // Move 3px towards center
            if (nx < center_x)
                nx += 3;
            else if (nx > center_x)
                nx -= 3;
            if (ny < center_y)
                ny += 3;
            else if (ny > center_y)
                ny -= 3;

            inner_highlight_points.push_back({nx, ny, std::max(0, p.radius - 3)});
        }

        gc.save();
        gc.clipPolygon(inner_highlight_points);
        // Bright reflection usually spans about 80% of the top half
        int h_height = static_cast<int>(half_height * 0.8);
        gc.fillLinearGradientRect(min_x, min_y + 2, poly_width, h_height, highlight, highlight2,
                                  true, 0);
        gc.restore();
    }
} // namespace horizon
