#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/SidebarItem.hpp>

namespace horizon
{
    SidebarItem::SidebarItem(const std::string &icon_name, const std::string &text) : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_fixed_size(30); // Height of the item
        set_spacing(10);
        set_margin(10); // Restore original margin

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name(icon_name);
        icon->set_icon_size(18);
        icon->set_fixed_size(18);
        m_icon_ptr = icon.get();
        add_child(std::move(icon));

        auto label = std::make_unique<Label>(text);
        label->set_font_size(13);
        label->set_vertical_alignment(VerticalAlignment::Middle); // Center text vertically
        m_label_ptr = label.get();
        add_child(std::move(label));

        set_focusable(true);

        when_focus.connect(
            [this](EventContext &)
            {
                // Note: Selection is now managed externally or via click
            });

        when_blur.connect(
            [this](EventContext &)
            {
                // Note: Blur doesn't clear selection anymore
            });
    }

    void SidebarItem::calculate_layout()
    {
        // Internal padding (Indentation)
        const int indentation = 25;

        // Update children positions manually to simulate indentation without height collapse
        // Micro-adjustment (+1px) to fix the "slightly up" visual misalignment
        m_start_draw_x = m_x + indentation;
        m_start_draw_y = m_y + 1;
        m_available_draw_width = m_width - indentation;
        m_available_draw_height = m_height - 1;

        Widget::calculate_layout();
    }

    Widget *SidebarItem::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled)
            return nullptr;

        // Check if any child (like an eject button) was hit first
        Widget *child_hit = Widget::hit_test(x, y);
        if (child_hit && child_hit != this)
            return child_hit;

        if (x < m_x || y < m_y || x >= m_x + m_width || y >= m_y + m_height)
            return nullptr;

        return this;
    }

    void SidebarItem::draw(GraphicsContext &gc)
    {
        bool selected = m_selected;

        auto *tm = theme_manager();
        Color sidebar_item_fg = tm->get_color("sidebar_item_fg");
        Color sidebar_item_fg2 = tm->get_color("sidebar_item_fg2");

        if (selected)
        {
            // Blue gradient selection (macOS Aqua style)
            Color c1(0.32f, 0.61f, 0.90f, 1.0f); // Top
            Color c2(0.11f, 0.45f, 0.81f, 1.0f); // Bottom
            gc.fillLinearGradientRect(m_x, m_y, m_width, m_height, c1, c2, true);

            if (m_label_ptr)
                m_label_ptr->set_text_color(sidebar_item_fg2);
        }
        else
        {
            if (m_label_ptr)
                m_label_ptr->set_text_color(sidebar_item_fg);
        }

        // Children (Icon, Label) are drawn by Widget::render
    }
} // namespace horizon
