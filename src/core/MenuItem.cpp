#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{

    MenuItem::MenuItem() : Widget()
    {
        auto content = std::make_unique<Label>();
        m_content = content.get();
        m_content->set_position_type(FREE);
        static_cast<Label *>(m_content)->set_vertical_alignment(VerticalAlignment::Middle);
        add_child(std::move(content));

        auto shortcut = std::make_unique<Label>();
        m_shortcut_label = shortcut.get();
        m_shortcut_label->set_position_type(FREE);
        m_shortcut_label->set_vertical_alignment(VerticalAlignment::Middle);
        m_shortcut_label->set_alignment(TextAlignment::Right);
        add_child(std::move(shortcut));

        set_size(200, 24); // Default menu item size

        when_mouse_enter.connect(
            [this](EventContext &)
            {
                set_selected(true);
                if (auto *p = dynamic_cast<Menu *>(parent()))
                {
                    p->set_active_submenu(m_submenu);
                }
            });

        when_mouse_leave.connect([this](EventContext &) { set_selected(false); });
    }

    MenuItem::MenuItem(const std::string &text) : MenuItem()
    {
        set_text(text);
    }

    void MenuItem::set_text(const std::string &text)
    {
        if (auto *label = dynamic_cast<Label *>(m_content))
        {
            label->set_text(text);
        }
    }

    const std::string &MenuItem::text() const
    {
        if (auto *label = dynamic_cast<Label *>(m_content))
        {
            return label->text();
        }
        static std::string empty;
        return empty;
    }

    void MenuItem::set_shortcut(const std::string &shortcut)
    {
        m_shortcut_text = shortcut;
        m_shortcut_label->set_text(shortcut);
    }

    void MenuItem::set_icon(const std::string &icon_path)
    {
        auto icon = std::make_unique<Icon>();
        m_icon = icon.get();
        m_icon->set_icon_name(icon_path);
        m_icon->set_icon_size(16);
        add_child(std::move(icon));
    }

    void MenuItem::set_has_submenu(bool has_submenu)
    {
        m_has_submenu = has_submenu;
    }

    void MenuItem::set_selected(bool selected)
    {
        if (m_selected != selected)
        {
            m_selected = selected;
            invalidate();
        }
    }

    void MenuItem::set_content_widget(std::unique_ptr<Widget> widget)
    {
        // This is a bit tricky since we already have a default content
        // We might want to remove the old one if we were doing this for real
        m_content = widget.get();
        m_content->set_position_type(FREE);
        add_child(std::move(widget));
    }

    void MenuItem::set_submenu(Menu *submenu)
    {
        m_submenu = submenu;
        set_has_submenu(submenu != nullptr);
    }

    void MenuItem::update_layout()
    {
        // Essential: Refresh m_start_draw_x/y
        Widget::calculate_layout();

        int icon_width = 24;
        int arrow_width = 20;
        int shortcut_width = m_shortcut_text.empty() ? 0 : 60;
        int padding = 10;

        int content_x = padding;
        if (m_icon)
        {
            m_icon->set_position(m_start_draw_x + padding + (icon_width - 16) / 2,
                                 m_start_draw_y + (m_height - 16) / 2);
            content_x += icon_width;
        }

        int available_content_width =
            m_width - content_x - shortcut_width - (m_has_submenu ? arrow_width : 0) - padding;

        m_content->set_position(m_start_draw_x + content_x, m_start_draw_y);
        m_content->set_size(available_content_width, m_height);

        if (!m_shortcut_text.empty() && m_shortcut_label)
        {
            m_shortcut_label->set_position(
                m_start_draw_x + m_width - arrow_width - shortcut_width - padding, m_start_draw_y);
            m_shortcut_label->set_size(shortcut_width, m_height);
        }

        m_content->set_application_recursive(application());
        if (m_shortcut_label)
            m_shortcut_label->set_application_recursive(application());
        if (m_icon)
            m_icon->set_application_recursive(application());
    }

    void MenuItem::draw(GraphicsContext &gc)
    {
        update_layout();

        if (m_selected)
        {
            // macOS selection gradient (Blue)
            Color c1(0.2f, 0.45f, 0.9f, 1.0f);
            Color c2(0.1f, 0.35f, 0.85f, 1.0f);
            gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_width, m_height, c1, c2,
                                      true);

            // White text for selected items
            if (auto *label = dynamic_cast<Label *>(m_content))
            {
                label->set_text_color({1.0f, 1.0f, 1.0f, 1.0f});
            }
            m_shortcut_label->set_text_color({1.0f, 1.0f, 1.0f, 0.8f});
        }
        else
        {
            // Default text color
            if (auto *label = dynamic_cast<Label *>(m_content))
            {
                label->set_text_color({0.0f, 0.0f, 0.0f, 1.0f});
            }
            m_shortcut_label->set_text_color({0.4f, 0.4f, 0.4f, 1.0f});
        }

        // Draw children (items) via base class
        Widget::draw(gc);

        if (m_has_submenu)
        {
            // Draw submenu arrow
            int arrow_size = 6;
            int ax = m_start_draw_x + m_width - 15;
            int ay = m_start_draw_y + (m_height - arrow_size * 2) / 2;

            gc.setColor(m_selected ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(0.2f, 0.2f, 0.2f, 1.0f));
            std::vector<PolygonPoint> points;
            points.push_back({ax, ay, 0});
            points.push_back({ax + arrow_size, ay + arrow_size, 0});
            points.push_back({ax, ay + arrow_size * 2, 0});
            gc.fillPolygon(points);
        }
    }

} // namespace horizon
