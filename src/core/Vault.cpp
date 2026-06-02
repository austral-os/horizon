#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Vault.hpp>
#include <horizon/WaylandWindow.hpp>

namespace horizon
{
    Vault::Vault() : Widget()
    {
        set_focusable(true);
        m_margin = 12;
        m_arrow_x = -1;
        m_arrow_y = -1;
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

    Widget *Vault::content() const
    {
        if (children().empty())
            return nullptr;
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
            if (content_w <= 0)
            {
                child->calculate_layout();
                content_w = child->width();
            }
            if (content_h <= 0)
            {
                child->calculate_layout();
                content_h = child->height();

                // If it's a vertical layout container, compute height from children dynamically
                if (content_h <= 0 && child->layout_type() == WIDGET_LAYOUT_VERTICAL)
                {
                    int total_h = child->margin() * 2;
                    int visible_count = 0;
                    for (const auto &c : child->children())
                    {
                        if (c->is_visible() && c->position_type() != FREE)
                        {
                            total_h += c->preferred_height(content_w);
                            visible_count++;
                        }
                    }
                    if (visible_count > 1)
                    {
                        total_h += child->spacing() * (visible_count - 1);
                    }
                    content_h = total_h;
                }
            }
            if (content_w <= 0)
                content_w = 200;
            if (content_h <= 0)
                content_h = 100;

            // Set our own size based on content PLUS margins for the arrow
            set_size(content_w + m_padding * 2 + m_margin * 2,
                     content_h + m_padding * 2 + m_margin * 2);

            // Pass 2: Place child at the correct ABSOLUTE position and re-layout
            child->set_position(m_start_draw_x + m_padding, m_start_draw_y + m_padding);
            child->set_size(content_w, content_h);
            child->calculate_layout();
        }
    }

    Widget *Vault::hit_test(int x, int y)
    {
        // Vault itself is a container, let's see if we hit children
        Widget *hit = Widget::hit_test(x, y);
        if (hit == this)
            return this; // We want the vault to be hit-testable for its background
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
        if (!application())
            return;

        CornerRadius radius(m_border_radius);

        Color menu_bg = theme_manager()->get_color("menu_bg");
        Color menu_border = theme_manager()->get_color("menu_border");
        
        float opacity = theme_manager()->get_menu_opacity();
        menu_bg.a = opacity;
        menu_border.a = opacity;

        int rect_w = m_width - m_margin * 2;
        int rect_h = m_height - m_margin * 2;

        // 1. Draw Border/Shadow
        gc.setColor(menu_border);
        gc.drawRect(m_start_draw_x, m_start_draw_y, rect_w, rect_h, radius, 1.0f);

        // 2. Fill Background
        gc.setColor(menu_bg);
        gc.fillRect(m_start_draw_x + 1, m_start_draw_y + 1, rect_w - 2, rect_h - 2, radius);

        // 3. Draw Integrated Arrow
        if (m_arrow_x >= 0 && m_arrow_y >= 0)
        {
            int arrow_size = 10;
            std::vector<PolygonPoint> arrow_points;
            int tip_x = m_arrow_x;
            int tip_y = m_arrow_y;

            // Determine which edge the arrow is on
            if (m_arrow_x < m_start_draw_x)
            { // Left
                arrow_points = {{tip_x, tip_y, 0},
                                {m_start_draw_x + 1, tip_y - arrow_size, 0},
                                {m_start_draw_x + 1, tip_y + arrow_size, 0}};
                gc.setColor(menu_bg);
                gc.fillPolygon(arrow_points);
                gc.setColor(menu_border);
                gc.drawLine(m_start_draw_x + 1, tip_y - arrow_size, tip_x, tip_y, 1.0f);
                gc.drawLine(tip_x, tip_y, m_start_draw_x + 1, tip_y + arrow_size, 1.0f);
            }
            else if (m_arrow_x > m_start_draw_x + rect_w)
            { // Right
                arrow_points = {{tip_x, tip_y, 0},
                                {m_start_draw_x + rect_w - 1, tip_y - arrow_size, 0},
                                {m_start_draw_x + rect_w - 1, tip_y + arrow_size, 0}};
                gc.setColor(menu_bg);
                gc.fillPolygon(arrow_points);
                gc.setColor(menu_border);
                gc.drawLine(m_start_draw_x + rect_w - 1, tip_y - arrow_size, tip_x, tip_y, 1.0f);
                gc.drawLine(tip_x, tip_y, m_start_draw_x + rect_w - 1, tip_y + arrow_size, 1.0f);
            }
            else if (m_arrow_y < m_start_draw_y)
            { // Top
                arrow_points = {{tip_x, tip_y, 0},
                                {tip_x - arrow_size, m_start_draw_y + 1, 0},
                                {tip_x + arrow_size, m_start_draw_y + 1, 0}};
                gc.setColor(menu_bg);
                gc.fillPolygon(arrow_points);
                gc.setColor(menu_border);
                gc.drawLine(tip_x - arrow_size, m_start_draw_y + 1, tip_x, tip_y, 1.0f);
                gc.drawLine(tip_x, tip_y, tip_x + arrow_size, m_start_draw_y + 1, 1.0f);
            }
            else if (m_arrow_y > m_start_draw_y + rect_h)
            { // Bottom
                arrow_points = {{tip_x, tip_y, 0},
                                {tip_x - arrow_size, m_start_draw_y + rect_h - 1, 0},
                                {tip_x + arrow_size, m_start_draw_y + rect_h - 1, 0}};
                gc.setColor(menu_bg);
                gc.fillPolygon(arrow_points);
                gc.setColor(menu_border);
                gc.drawLine(tip_x - arrow_size, m_start_draw_y + rect_h - 1, tip_x, tip_y, 1.0f);
                gc.drawLine(tip_x, tip_y, tip_x + arrow_size, m_start_draw_y + rect_h - 1, 1.0f);
            }
        }
    }

} // namespace horizon
