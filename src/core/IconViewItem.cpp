#include <horizon/Icon.hpp>
#include <horizon/IconViewItem.hpp>
#include <horizon/Label.hpp>

namespace horizon
{
    IconViewItem::IconViewItem() : Widget()
    {
        auto icon = std::make_unique<Icon>();
        icon->set_position_type(FREE);
        icon->set_icon_size(m_icon_size);
        icon->set_icon_name("folder");
        m_icon_ptr = icon.get();
        add_child(std::move(icon));

        auto label = std::make_unique<Label>();
        label->set_position_type(FREE);
        label->set_alignment(TextAlignment::Center);
        label->set_vertical_alignment(VerticalAlignment::Top);
        label->set_font_size(BASE_FONT_SIZE);
        m_label_ptr = label.get();
        add_child(std::move(label));

        m_position_type = FREE;
    }

    void IconViewItem::set_text(const std::string &text)
    {
        m_label_ptr->set_text(text);
        invalidate();
    }

    const std::string &IconViewItem::text() const
    {
        return m_label_ptr->text();
    }

    void IconViewItem::set_icon_name(const std::string &icon_name)
    {
        m_icon_ptr->set_icon_name(icon_name);
        invalidate();
    }

    const std::string &IconViewItem::icon_name() const
    {
        return m_icon_ptr->icon_name();
    }

    void IconViewItem::set_zoom(float zoom)
    {
        m_zoom = zoom;
        m_icon_size = static_cast<int>(BASE_ICON_SIZE * m_zoom);
        m_icon_ptr->set_icon_size(m_icon_size);
        m_label_ptr->set_font_size(static_cast<int>(BASE_FONT_SIZE * m_zoom));
        invalidate();
    }

    float IconViewItem::zoom() const
    {
        return m_zoom;
    }

    void IconViewItem::calculate_layout()
    {
        // Layout: Icon at top, Label at bottom
        int padding = static_cast<int>(4 * m_zoom);
        int icon_y = padding;
        int label_y = icon_y + m_icon_size + padding;

        m_icon_ptr->set_position(m_x + (m_width - m_icon_size) / 2, m_y + icon_y);
        m_icon_ptr->set_size(m_icon_size, m_icon_size);

        m_label_ptr->set_position(m_x + padding, m_y + label_y);
        m_label_ptr->set_size(m_width - 2 * padding, m_height - label_y - padding);
    }

    void IconViewItem::draw(GraphicsContext &gc)
    {
        if (is_hovered())
        {
            gc.setColor(Color(0.8f, 0.8f, 0.9f, 0.5f));
            gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(6));
        }
        else if (is_pressed())
        {
            gc.setColor(Color(0.7f, 0.7f, 0.8f, 0.7f));
            gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(6));
        }

        // Children (Icon and Label) will be rendered by the base Widget::render
    }

    int IconViewItem::preferred_width() const
    {
        return static_cast<int>(80 * m_zoom);
    }

    int IconViewItem::preferred_height() const
    {
        return static_cast<int>(100 * m_zoom);
    }
} // namespace horizon
