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
        set_margin(0);

        auto scroll_area = std::make_unique<ScrollArea>();
        m_scroll_area = scroll_area.get();

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_spacing(0);
        content->set_margin(0); // REMOVED MARGIN for edge-to-edge selection
        m_content_container = content.get();

        m_scroll_area->set_content(std::move(content));
        add_child(std::move(scroll_area));
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

        header->set_fixed_size(25);
        header->set_vertical_alignment(VerticalAlignment::Middle);

        m_groups[name] = group_container.get();

        group_container->add_child(std::move(header));
        m_content_container->add_child(std::move(group_container));
    }

    void Sidebar::add_item(const std::string &group_name, std::unique_ptr<Widget> item)
    {
        auto it = m_groups.find(group_name);
        if (it != m_groups.end())
        {
            it->second->add_child(std::move(item));
        }
        invalidate();
    }

    void Sidebar::render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force)
    {
        Widget::render(gc, cx, cy, cw, ch, force);
    }

    void Sidebar::calculate_layout()
    {
        if (!m_content_container || !m_scroll_area)
            return;

        // Ensure ScrollArea fills the Sidebar
        m_scroll_area->set_position(m_x, m_y);
        m_scroll_area->set_size(m_width, m_height);

        int total_height = 0;
        int width = m_width;

        // Calculate height based on children
        for (const auto &group : m_content_container->children())
        {
            if (!group->is_visible())
                continue;

            int group_height = 0;
            int count = 0;
            for (const auto &item : group->children())
            {
                if (!item->is_visible())
                    continue;
                if (item->fixed_size() > 0)
                {
                    group_height += item->fixed_size() + group->spacing();
                    count++;
                }
            }
            if (count > 0)
                group_height -= group->spacing();

            // FIX: Set fixed_size so parent layout (VPanel) knows how much space to give
            group->set_fixed_size(group_height);
            group->set_size(width, group_height); // Using full width

            total_height += group_height + m_content_container->spacing();
        }

        if (!m_content_container->children().empty())
            total_height -= m_content_container->spacing();

        // Include content container margins
        total_height += m_content_container->margin() * 2;

        m_content_container->set_fixed_size(total_height);
        m_content_container->set_size(width, total_height);

        Widget::calculate_layout();
    }

    void Sidebar::draw(GraphicsContext &gc)
    {
        // Slight blueish background for the sidebar (Source List look)
        gc.setColor(0.875f, 0.902f, 0.933f, 1.0f);
        gc.fillRect(m_x, m_y, m_width, m_height);

        // Vertical separator line on the right
        gc.setColor(0.85f, 0.85f, 0.85f, 1.0f);
        gc.drawLine(m_x + m_width - 1, m_y, m_x + m_width - 1, m_y + m_height);
    }
} // namespace horizon
