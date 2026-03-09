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
        set_margin(10); // MUST be 0 to allow vertical centering using full 30px height

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
                if (m_label_ptr)
                    m_label_ptr->set_text_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
            });

        when_blur.connect(
            [this](EventContext &)
            {
                if (m_label_ptr)
                    m_label_ptr->set_text_color(Color(0.2f, 0.2f, 0.2f, 1.0f));
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

        if (x < m_x || y < m_y || x >= m_x + m_width || y >= m_y + m_height)
            return nullptr;

        return this;
    }

    void SidebarItem::draw(GraphicsContext &gc)
    {
        bool selected = has_focus();

        if (is_hovered() && !selected)
        {
            gc.setColor(0.9f, 0.9f, 0.9f, 0.5f);
            gc.fillRect(m_x, m_y, m_width, m_height);
        }

        if (selected)
        {
            // Blue gradient selection (macOS Aqua style)
            Color c1(0.32f, 0.61f, 0.90f, 1.0f); // Top
            Color c2(0.11f, 0.45f, 0.81f, 1.0f); // Bottom
            gc.fillLinearGradientRect(m_x, m_y, m_width, m_height, c1, c2, true);
        }

        // Children (Icon, Label) are drawn by Widget::render
    }
} // namespace horizon
