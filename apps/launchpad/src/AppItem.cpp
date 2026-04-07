#include "AppItem.hpp"
#include <horizon/GraphicsContext.hpp>

namespace horizon
{

    AppItem::AppItem() : Widget()
    {
        auto icon = std::make_unique<Icon>();
        icon->set_position_type(FREE);
        m_icon_ptr = icon.get();
        add_child(std::move(icon));

        auto label = std::make_unique<Label>();
        label->set_position_type(FREE);
        label->set_alignment(TextAlignment::Center);
        label->set_vertical_alignment(VerticalAlignment::Top);
        m_label_ptr = label.get();
        add_child(std::move(label));

        m_position_type = FREE;

        when_mouse_enter.connect([this](EventContext &) { invalidate(); });
        when_mouse_leave.connect([this](EventContext &) { invalidate(); });
    }

    void AppItem::set_data(const AppData &data, float zoom, bool selected)
    {
        m_zoom = zoom;
        m_selected = selected;
        m_label_ptr->set_text(data.name);

        m_icon_size = static_cast<int>(64 * m_zoom);
        m_icon_ptr->set_icon_name(data.icon_name);
        m_icon_ptr->set_icon_size(m_icon_size);
        invalidate();
    }

    void AppItem::set_font_size(int size)
    {
        m_label_ptr->set_font_size(size);
        invalidate();
    }

    int AppItem::preferred_height(int width) const
    {
        int padding = static_cast<int>(10 * m_zoom);
        int gap = 8;
        int label_h = m_label_ptr->preferred_height(width - 10);
        return padding + m_icon_size + gap + label_h + padding;
    }

    void AppItem::calculate_layout()
    {
        int padding = static_cast<int>(10 * m_zoom);
        int icon_y = padding;

        m_icon_ptr->set_position(m_x + (m_width - m_icon_size) / 2, m_y + icon_y);
        m_icon_ptr->set_size(m_icon_size, m_icon_size);

        int label_y = icon_y + m_icon_size + 8;
        m_label_ptr->set_position(m_x + 5, m_y + label_y);
        m_label_ptr->set_size(m_width - 10, m_height - label_y - 5);
    }

    void AppItem::draw(GraphicsContext &gc)
    {
        if (m_is_hovered || m_selected)
        {
            Color hover_color = Color(1.0f, 1.0f, 1.0f, 0.15f);
            if (m_selected)
                hover_color = Color(1.0f, 1.0f, 1.0f, 0.3f);

            gc.setColor(hover_color);
            gc.fillRect(m_x + 5, m_y + 5, m_width - 10, m_height - 10, CornerRadius(12));
        }

        m_label_ptr->set_text_color(Color(1.0f, 1.0f, 1.0f, 1.0f)); // White text for launchpad
    }

} // namespace horizon
