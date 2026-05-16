#include <horizon/Notification.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Application.hpp>
#include <horizon/ThemeManager.hpp>
#include <algorithm>

namespace horizon
{

    Notification::Notification()
    {
        // 1. Configure the notification container
        // 1. Configure the notification container
        set_border_radius(10);
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);
        set_margin(m_internal_padding);

        // 2. Create the icon widget
        auto icon = std::make_unique<Icon>();
        m_icon_widget = icon.get();
        m_icon_widget->set_icon_size(m_icon_size);
        m_icon_widget->set_fixed_size(m_icon_size);
        m_icon_widget->set_use_theme_colors(true);
        m_icon_widget->set_vertical_alignment(VerticalAlignment::Top);
        m_icon_widget->set_visible(false); // Initially hidden
        add_child(std::move(icon));

        // 3. Create the message label
        auto label = std::make_unique<Label>();
        m_label_widget = label.get();
        m_label_widget->set_alignment(TextAlignment::Left);
        m_label_widget->set_vertical_alignment(VerticalAlignment::Top);
        add_child(std::move(label));
    }

    void Notification::set_icon_name(const std::string &name)
    {
        if (name.empty())
        {
            m_icon_widget->set_visible(false);
            m_icon_widget->set_icon_name("");
        }
        else
        {
            m_icon_widget->set_icon_name(name);
            m_icon_widget->set_visible(true);
        }
        invalidate();
    }

    void Notification::set_message(const std::string &message)
    {
        m_label_widget->set_text(message);
        invalidate();
    }

    void Notification::set_notification(const std::string &icon_name, const std::string &message)
    {
        set_icon_name(icon_name);
        set_message(message);
    }

    void Notification::set_fixed_width(int width)
    {
        m_fixed_width = width;
        invalidate();
    }

    const std::string &Notification::message() const
    {
        return m_label_widget->text();
    }

    const std::string &Notification::icon_name() const
    {
        return m_icon_widget->icon_name();
    }

    int Notification::preferred_width() const
    {
        if (m_fixed_width > 0)
            return m_fixed_width;

        int width = (m_margin * 2);
        if (m_icon_widget->is_visible())
        {
            width += m_icon_widget->fixed_size() + m_spacing;
        }

        width += m_label_widget->preferred_width();
        return width;
    }

    int Notification::preferred_height() const
    {
        return preferred_height(m_width);
    }

    int Notification::preferred_height(int width) const
    {
        int available_width = width - (m_margin * 2);
        if (available_width < 0)
            available_width = 0;
            
        if (m_icon_widget->is_visible())
        {
            available_width -= (m_icon_widget->fixed_size() + m_spacing);
            if (available_width < 0)
                available_width = 0;
        }

        int label_h = m_label_widget->preferred_height(available_width);
        int icon_h = m_icon_widget->is_visible() ? m_icon_widget->fixed_size() : 0;

        return std::max(label_h, icon_h) + (m_margin * 2);
    }

    void Notification::calculate_layout()
    {
        // Use the base class logic which already handles children if m_layout_type is WIDGET_LAYOUT_HORIZONTAL
        // It will correctly skip hidden children (like the icon if not set).
        Widget::calculate_layout();
    }

    void Notification::draw(GraphicsContext &ctx)
    {
        if (application() && application()->theme_manager)
        {
            auto *tm = application()->theme_manager.get();
            Color bg = tm->get_color("notification_bg");
            bg.a = 0.8f;
            set_background_color(bg);

            Color fg = tm->get_color("notification_fg");
            m_label_widget->set_text_color(fg);
        }

        // Simple rectangular background with transparency and rounded corners
        // Widget::draw already does this if background_color and border_radius are set.
        Widget::draw(ctx);
    }

} // namespace horizon
