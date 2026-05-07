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
        m_start_draw_x = m_x + m_margin;
        m_start_draw_y = m_y + m_margin;

        Widget *child = content();
        if (child)
        {
            child->set_position_type(FREE);

            // Pass 1: Get preferred/content dimensions without caring about position
            int content_w = child->preferred_width();
            int content_h = child->preferred_height();
            if (content_w <= 0) { child->calculate_layout(); content_w = child->width(); }
            if (content_h <= 0) { child->calculate_layout(); content_h = child->height(); }
            if (content_w <= 0) content_w = 200;
            if (content_h <= 0) content_h = 100;

            // Set our own size based on content
            set_size(content_w + m_padding * 2, content_h + m_padding * 2);

            // Pass 2: Place child at the correct ABSOLUTE position and re-layout
            // so all grandchildren (Labels, Sliders, etc.) end up at the right coordinates
            child->set_position(m_start_draw_x + m_padding, m_start_draw_y + m_padding);
            child->set_size(content_w, content_h);
            child->calculate_layout(); // This call places grandchildren correctly
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
        
        // Horizon Menu style (macOS inspired)
        // We use full rounding for Vault as it's a bubble, 
        // but the colors/borders must match Menu.
        CornerRadius radius(m_border_radius);
        Color menu_bg = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color menu_border = Color(0.7f, 0.7f, 0.7f, 0.8f);

        // 1. Draw Border/Shadow
        gc.setColor(menu_border);
        gc.drawRect(m_start_draw_x, m_start_draw_y, m_width, m_height, radius, 1.0f);

        // 2. Fill Background
        gc.setColor(menu_bg);
        gc.fillRect(m_start_draw_x + 1, m_start_draw_y + 1, m_width - 2, m_height - 2, radius);

        // 3. Draw Integrated Arrow
        int arrow_size = 10;
        int ax = m_start_draw_x + m_width / 2;
        int ay = m_start_draw_y;

        std::vector<PolygonPoint> arrow_points = {
            {(float)ax, (float)ay - 8, 0},              // Tip
            {(float)ax - arrow_size, (float)ay + 1, 0}, // Base Left
            {(float)ax + arrow_size, (float)ay + 1, 0}  // Base Right
        };

        gc.setColor(menu_bg);
        gc.fillPolygon(arrow_points);
        
        gc.setColor(menu_border);
        gc.drawLine(ax - arrow_size, ay + 1, ax, ay - 8, 1.0f);
        gc.drawLine(ax, ay - 8, ax + arrow_size, ay + 1, 1.0f);
    }

} // namespace horizon
