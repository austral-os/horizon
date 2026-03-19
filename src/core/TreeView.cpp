#include <horizon/TreeView.hpp>
#include <horizon/GraphicsContext.hpp>

namespace horizon
{
    TreeView::TreeView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_spacing(0);
        set_margin(0);

        auto scroll_area = std::make_unique<ScrollArea>();
        m_scroll_area = scroll_area.get();

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_spacing(0);
        content->set_margin(0);
        m_content_container = content.get();

        m_scroll_area->set_content(std::move(content));
        add_child(std::move(scroll_area));
    }

    void TreeView::add_root_item(std::unique_ptr<TreeViewItem> item)
    {
        m_content_container->add_child(std::move(item));
        invalidate();
    }

    void TreeView::clear_root_items()
    {
        m_content_container->clear_children();
        m_selected_item = nullptr;
        invalidate();
    }

    void TreeView::set_selected_item(TreeViewItem *item)
    {
        if (m_selected_item == item)
            return;

        if (m_selected_item)
        {
            m_selected_item->set_selected(false);
        }

        m_selected_item = item;

        if (m_selected_item)
        {
            m_selected_item->set_selected(true);
        }

        when_item_selected.run(item);
        invalidate();
    }

    void TreeView::calculate_layout()
    {
        if (!m_content_container || !m_scroll_area)
            return;

        // Base layout handles m_scroll_area positioning (FILL)
        Widget::calculate_layout();

        // Update content container based on items
        int total_h = 0;
        for (const auto &child : m_content_container->children())
        {
            auto *item = dynamic_cast<TreeViewItem *>(child.get());
            if (item && item->is_visible())
            {
                int item_h = item->total_height();
                item->set_fixed_size(item_h);
                total_h += item_h;
            }
        }

        m_content_container->set_fixed_size(total_h);
        // Important: use m_available_draw_width from base calculate_layout
        m_content_container->set_size(m_available_draw_width, total_h);
        
        // Let scroll area re-evaluate scrollbars
        m_scroll_area->calculate_layout();
    }

    void TreeView::draw(GraphicsContext &gc)
    {
        gc.setColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        gc.fillRect(m_x, m_y, m_width, m_height);
    }
} // namespace horizon
