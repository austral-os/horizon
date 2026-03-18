#include <algorithm>
#include <horizon/GraphicsContext.hpp>
#include <horizon/GroupedIconsView.hpp>

namespace horizon
{
    // --- GroupIconItemWidget ---

    GroupIconItemWidget::GroupIconItemWidget(GroupedIconsView *view, const GroupedIconItem &item)
        : Widget(), m_view(view), m_item_data(item)
    {
        set_position_type(FREE); // Critical: Avoid size/position overwrite in grid

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name(item.icon_name);
        icon->set_icon_size(32);
        icon->set_horizontal_alignment(TextAlignment::Center);
        m_icon = icon.get();
        add_child(std::move(icon));

        auto label = std::make_unique<Label>(item.label);
        label->set_alignment(TextAlignment::Center);
        label->set_font_size(11);
        m_label = label.get();
        add_child(std::move(label));

        when_click.connect(
            [this](MouseButtonEventContext &)
            {
                if (m_view)
                {
                    m_view->when_item_click.run(m_item_data);
                }
            });

        when_dbl_click.connect(
            [this](MouseButtonEventContext &)
            {
                if (m_view)
                {
                    m_view->when_item_dbl_click.run(m_item_data);
                }
            });

        set_cursor_type(CursorType::Pointer);
    }

    void GroupIconItemWidget::calculate_layout()
    {
        // Use base layout
        Widget::calculate_layout();
    }

    void GroupIconItemWidget::draw(GraphicsContext &gc)
    {
        /*if (is_hovered())
        {
            gc.setColor(0.9f, 0.9f, 0.9f, 0.5f);
            gc.fillRect(x(), y(), width(), height());
        }*/
        Widget::draw(gc);
    }

    int GroupIconItemWidget::preferred_height(int width) const
    {
        return 32 + 10 + m_label->preferred_height(width) + 10;
    }

    // --- GroupGrid ---

    GroupGrid::GroupGrid() : Widget()
    {
        // Grid layout needs custom logic
    }

    void GroupGrid::calculate_layout()
    {
        Widget::calculate_layout();

        if (width() <= 0)
            return;

        int columns = std::max(1, width() / (m_item_width + m_grid_spacing));
        int row_height = 0;
        for (auto &child : children())
        {
            row_height = std::max(row_height, child->preferred_height(m_item_width));
        }

        for (size_t i = 0; i < children().size(); ++i)
        {
            int col = i % columns;
            int row = i / columns;

            // Use ABSOLUTE coordinates
            int child_x = x() + col * (m_item_width + m_grid_spacing);
            int child_y = y() + row * (row_height + m_grid_spacing);

            children()[i]->set_position(child_x, child_y);
            children()[i]->set_size(m_item_width, row_height);
            children()[i]->calculate_layout();
        }
    }

    int GroupGrid::preferred_height(int width) const
    {
        if (width <= 0)
            return 0;
        if (children().empty())
            return 0;

        int columns = std::max(1, width / (m_item_width + m_grid_spacing));
        int rows = (children().size() + columns - 1) / columns;

        int row_height = 0;
        for (auto &child : children())
        {
            row_height = std::max(row_height, child->preferred_height(m_item_width));
        }

        return rows * row_height + (rows - 1) * m_grid_spacing;
    }

    // --- GroupSeparator ---

    GroupSeparator::GroupSeparator() : Widget()
    {
        // Default layout
    }

    void GroupSeparator::draw(GraphicsContext &gc)
    {
        gc.setColor(0.8f, 0.8f, 0.8f, 1.0f);
        gc.drawLine(x(), y() + height() / 2, x() + width(), y() + height() / 2, 1.0f);
    }

    int GroupSeparator::preferred_height(int width) const
    {
        return 10;
    }

    // --- GroupContainer ---

    GroupContainer::GroupContainer() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
    }

    void GroupContainer::calculate_layout()
    {
        Widget::calculate_layout();
    }

    int GroupContainer::preferred_height(int width) const
    {
        int h = margin() * 2;
        int avail_width = width - (margin() * 2);
        if (children().empty())
            return h;

        for (auto &child : children())
        {
            h += child->preferred_height(avail_width) + spacing();
        }
        return h - spacing();
    }

    // --- GroupedIconsView ---

    GroupedIconsView::GroupedIconsView() : Widget()
    {
        set_background_color(Color(0.95f, 0.95f, 0.95f, 1.0f));

        auto scroll_area = std::make_unique<ScrollArea>();
        scroll_area->set_position_type(FREE);
        m_scroll_area = scroll_area.get();

        auto content_pane = std::make_unique<Widget>();
        content_pane->set_position_type(FREE);
        content_pane->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        m_content_pane = content_pane.get();

        m_scroll_area->set_content(std::move(content_pane));
        add_child(std::move(scroll_area));
    }

    void GroupedIconsView::add_group(const IconGroup &group)
    {
        m_groups.push_back(group);
        rebuild_ui();
    }

    void GroupedIconsView::set_alternate_colors(const Color &c1, const Color &c2)
    {
        m_alt_color1 = c1;
        m_alt_color2 = c2;
        m_has_alt_colors = true;
        rebuild_ui();
    }

    void GroupedIconsView::clear_groups()
    {
        m_groups.clear();
        rebuild_ui();
    }

    void GroupedIconsView::rebuild_ui()
    {
        if (!m_content_pane)
            return;
        m_content_pane->clear_children();

        for (size_t i = 0; i < m_groups.size(); ++i)
        {
            const auto &group = m_groups[i];

            auto group_container = std::make_unique<GroupContainer>();
            group_container->set_margin(15);
            group_container->set_spacing(10);

            if (m_has_alt_colors)
            {
                group_container->set_background_color((i % 2 == 0) ? m_alt_color1 : m_alt_color2);
            }

            // Header
            auto header = std::make_unique<Label>(group.title);
            header->set_font_weight(FONT_WEIGHT_BOLD);
            header->set_font_size(14);
            header->set_margin(5);
            group_container->add_child(std::move(header));

            // Grid
            auto grid = std::make_unique<GroupGrid>();
            for (const auto &item : group.items)
            {
                grid->add_child(std::make_unique<GroupIconItemWidget>(this, item));
            }
            group_container->add_child(std::move(grid));

            // Separator (except for the last group)
            if (i < m_groups.size() - 1 && !m_has_alt_colors)
            {
                group_container->add_child(std::make_unique<GroupSeparator>());
            }

            m_content_pane->add_child(std::move(group_container));
        }

        invalidate();
    }

    void GroupedIconsView::calculate_layout()
    {
        Widget::calculate_layout();

        if (m_scroll_area)
        {
            m_scroll_area->set_position(x(), y());
            m_scroll_area->set_size(width(), height());
        }

        if (m_content_pane)
        {
            int avail_width = width() - (m_content_pane->margin() * 2); // Scrollbar gutter
            if (avail_width < 100)
                avail_width = 100;

            int total_height = m_content_pane->margin() * 2;

            for (auto &child : m_content_pane->children())
            {
                // Recursively update fixed sizes for the group container and its sub-widgets
                auto gc = dynamic_cast<GroupContainer *>(child.get());
                if (gc)
                {
                    for (auto &sub : gc->children())
                    {
                        sub->set_fixed_size(sub->preferred_height(avail_width));
                    }
                    gc->set_fixed_size(gc->preferred_height(avail_width));
                }
                total_height += child->fixed_size() + m_content_pane->spacing();
            }

            int final_height = std::max(height(), total_height);
            if (m_content_pane->height() != final_height || m_content_pane->width() != width())
            {
                m_content_pane->set_size(width(), final_height);
            }
            // Let the base render loop handle m_content_pane->calculate_layout()
        }
    }

} // namespace horizon
