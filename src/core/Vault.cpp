#include <horizon/Vault.hpp>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Logger.hpp>

namespace horizon
{
    Vault::Vault() : Widget()
    {
        set_focusable(true);
    }

    void Vault::set_content(std::unique_ptr<Widget> content)
    {
        clear_children();
        if (content)
        {
            content->set_position(m_padding, m_padding);
            add_child(std::move(content));
        }
        calculate_layout();
    }

    Widget* Vault::content() const
    {
        if (children().empty()) return nullptr;
        return children()[0].get();
    }

    void Vault::calculate_layout()
    {
        Widget *child = content();
        if (child)
        {
            child->set_position_type(FREE);
            child->calculate_layout();
            int content_w = child->width();
            int content_h = child->height();

            // Prefer preferred size over current size to avoid feedback loops
            if (child->preferred_width() > 0) content_w = child->preferred_width();
            if (child->preferred_height() > 0) content_h = child->preferred_height();
            
            // If still 0, use a reasonable minimum
            if (content_w <= 0) content_w = 200;
            if (content_h <= 0) content_h = 100;

            set_size(content_w + m_padding * 2, content_h + m_padding * 2);
            child->set_position(m_padding, m_padding);
            child->set_size(content_w, content_h);
        }
    }

    Widget* Vault::hit_test(int x, int y)
    {
        // Vault itself is a container, let's see if we hit children
        Widget *hit = Widget::hit_test(x, y);
        if (hit == this) return this; // We want the vault to be hit-testable for its background
        return hit;
    }

    void Vault::set_arrow_position(int x, int y)
    {
        m_arrow_x = x;
        m_arrow_y = y;
        invalidate();
    }

    void Vault::draw(GraphicsContext &gc)
    {
        if (!application()) return;
        
        // Premium Midnight Palette (Opaque for maximum compatibility)
        Color bg_top = Color(0.20f, 0.22f, 0.26f, 1.0f);
        Color bg_bottom = Color(0.12f, 0.14f, 0.16f, 1.0f);
        Color border_color = Color(0.40f, 0.42f, 0.48f, 1.0f);

        // 1. Draw Main Bubble with Linear Gradient
        gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_width, m_height, 
                                 bg_top, bg_bottom, true, CornerRadius(m_border_radius));

        // 2. Draw Polished Border
        gc.setColor(border_color);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height, CornerRadius(m_border_radius), 1.2f);

        // 3. Draw Integrated Arrow
        int arrow_size = 10;
        int ax = m_start_draw_x + m_width / 2;
        int ay = m_start_draw_y;

        std::vector<PolygonPoint> arrow_points = {
            {(float)ax, (float)ay - 8, 0},              // Tip
            {(float)ax - arrow_size, (float)ay + 1, 0}, // Base Left
            {(float)ax + arrow_size, (float)ay + 1, 0}  // Base Right
        };

        gc.setColor(bg_top);
        gc.fillPolygon(arrow_points);
        
        gc.setColor(border_color);
        gc.drawLine(ax - arrow_size, ay + 1, ax, ay - 8, 1.2f);
        gc.drawLine(ax, ay - 8, ax + arrow_size, ay + 1, 1.2f);
    }

} // namespace horizon
