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
        set_margin(5);

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name(icon_name);
        icon->set_icon_size(18);
        icon->set_fixed_size(18);
        m_icon_ptr = icon.get();
        add_child(std::move(icon));

        auto label = std::make_unique<Label>(text);
        label->set_font_size(13);
        m_label_ptr = label.get();
        add_child(std::move(label));
    }

    void SidebarItem::draw(GraphicsContext &gc)
    {
        if (is_hovered())
        {
            gc.setColor(0.9f, 0.9f, 0.9f, 1.0f);
            gc.fillRect(m_x, m_y, m_width, m_height, {4});
        }

        if (has_focus())
        {
            gc.setColor(0.8f, 0.9f, 1.0f, 1.0f);
            gc.fillRect(m_x, m_y, m_width, m_height, {4});
        }

        // Children (Icon, Label) are drawn by Widget::render
    }
} // namespace horizon
