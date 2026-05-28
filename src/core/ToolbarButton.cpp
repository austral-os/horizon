#include <horizon/Application.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/ToolbarButton.hpp>

namespace horizon
{
    ToolbarButton::ToolbarButton(const std::string &title, const std::string &icon_name,
                                 int icon_size)
        : m_title(title), m_icon_name(icon_name), m_icon_size(icon_size)
    {
        set_fixed_size(64);
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_spacing(0);

        auto icon_widget = std::make_unique<Icon>();
        icon_widget->set_icon_name(m_icon_name);
        if (m_icon_size > 0)
        {
            icon_widget->set_icon_size(m_icon_size);
        }

        icon_widget->set_margin(4);
        add_child(std::move(icon_widget));

        if (!m_title.empty())
        {
            auto label_widget = std::make_unique<Label>(m_title);
            label_widget->set_font_size(10);
            label_widget->set_alignment(TextAlignment::Center);
            label_widget->set_height(12);
            add_child(std::move(label_widget));
        }
    }

    void ToolbarButton::set_active(bool active)
    {
        if (m_active != active)
        {
            m_active = active;
            invalidate();
        }
    }

    void ToolbarButton::set_title(const std::string &title)
    {
        bool was_empty = m_title.empty();
        m_title = title;

        Label *label = nullptr;
        for (auto &child : children())
        {
            if (auto l = dynamic_cast<Label *>(child.get()))
            {
                label = l;
                break;
            }
        }

        if (m_title.empty())
        {
            if (label)
                remove_child(label);
        }
        else
        {
            if (label)
            {
                label->set_text(m_title);
            }
            else
            {
                auto label_widget = std::make_unique<Label>(m_title);
                label_widget->set_font_size(10);
                label_widget->set_alignment(TextAlignment::Center);
                label_widget->set_height(12);
                label_widget->set_text_color(m_text_color);
                add_child(std::move(label_widget));
            }
        }
        invalidate();
    }

    void ToolbarButton::set_icon_name(const std::string &icon_name)
    {
        m_icon_name = icon_name;
        for (auto &child : children())
        {
            if (auto icon = dynamic_cast<Icon *>(child.get()))
            {
                icon->set_icon_name(icon_name);
                break;
            }
        }
        invalidate();
    }

    void ToolbarButton::set_icon_size(int size)
    {
        m_icon_size = size;
        if (m_icon_size > 0)
        {
            for (auto &child : children())
            {
                if (auto icon = dynamic_cast<Icon *>(child.get()))
                {
                    icon->set_icon_size(size);
                    break;
                }
            }
            invalidate();
        }
    }

    void ToolbarButton::set_text_color(Color color)
    {
        m_text_color = color;
        for (auto &child : children())
        {
            if (auto label = dynamic_cast<Label *>(child.get()))
            {
                label->set_text_color(color);
                break;
            }
        }
    }

    void ToolbarButton::draw(GraphicsContext &gc)
    {
        auto *tm = theme_manager();

        if (is_enabled() && (m_active || m_is_hovered))
        {
            Color highlight = tm->get_color("titlebar_bg2");
            if (m_active)
            {
                highlight.a = 0.4f;
            }
            else
            {
                highlight.a = 0.2f;
            }
            gc.setColor(highlight);
            gc.fillRect(m_start_draw_x + 2, m_start_draw_y + 2, m_width - 4, m_height - 4, 6);
        }
    }

    void ToolbarButton::calculate_layout()
    {
        Widget::calculate_layout();

        if (m_title.empty() && !children().empty())
        {
            int total_children_height = 0;
            for (auto &child : children())
            {
                if (child->is_visible())
                {
                    total_children_height += child->height();
                }
            }
            if (total_children_height < m_height)
            {
                m_start_draw_y = m_y + (m_height - total_children_height) / 2;
            }
        }
    }
} // namespace horizon
