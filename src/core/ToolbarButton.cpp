#include <horizon/ToolbarButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Application.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    ToolbarButton::ToolbarButton(const std::string &title, const std::string &icon_name)
        : m_title(title), m_icon_name(icon_name)
    {
        set_fixed_size(64);
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_spacing(0);

        auto icon_widget = std::make_unique<Icon>();
        icon_widget->set_icon_name(m_icon_name);
        icon_widget->set_icon_size(32);
        icon_widget->set_margin(4);
        add_child(std::move(icon_widget));

        auto label_widget = std::make_unique<Label>(m_title);
        label_widget->set_font_size(10);
        label_widget->set_alignment(TextAlignment::Center);
        label_widget->set_height(12);
        add_child(std::move(label_widget));
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
        m_title = title;
        for (auto &child : children())
        {
            if (auto label = dynamic_cast<Label *>(child.get()))
            {
                label->set_text(title);
                break;
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
        auto *tm = application()->theme_manager.get();

        if (is_enabled() && (m_active || m_is_hovered))
        {
            Color highlight = tm->get_color("titlebar_bg2");
            if (m_active) {
                highlight.a = 0.4f;
            } else {
                highlight.a = 0.2f;
            }
            gc.setColor(highlight);
            gc.fillRect(m_start_draw_x + 2, m_start_draw_y + 2, m_width - 4, m_height - 4, 6);
        }
    }
} // namespace horizon
