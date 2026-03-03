#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Label.hpp>
#include <horizon/Sidebar.hpp>

namespace horizon
{
    Sidebar::Sidebar() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_spacing(0);
        set_margin(10);
    }

    void Sidebar::add_group(const std::string &name)
    {
        // Container for the group (Header + Items)
        auto group_container = std::make_unique<Widget>();
        group_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        group_container->set_spacing(2);
        group_container->set_margin(0);

        // Header Label
        auto header = std::make_unique<Label>(name);
        header->set_font_size(11);
        header->set_font_weight(FONT_WEIGHT_BOLD);
        header->set_text_color(Color(0.5f, 0.5f, 0.5f, 1.0f));
        header->set_margin(5);
        header->set_fixed_size(25);

        m_groups[name] = group_container.get();

        group_container->add_child(std::move(header));
        add_child(std::move(group_container));
    }

    void Sidebar::add_item(const std::string &group_name, std::unique_ptr<Widget> item)
    {
        auto it = m_groups.find(group_name);
        if (it != m_groups.end())
        {
            it->second->add_child(std::move(item));
        }
    }

    void Sidebar::draw(GraphicsContext &gc)
    {
        // Slight background for the sidebar
        gc.setColor(0.95f, 0.95f, 0.95f, 1.0f);
        gc.fillRect(m_x, m_y, m_width, m_height);

        // Vertical separator line on the right
        gc.setColor(0.85f, 0.85f, 0.85f, 1.0f);
        gc.drawLine(m_x + m_width - 1, m_y, m_x + m_width - 1, m_y + m_height);
    }
} // namespace horizon
