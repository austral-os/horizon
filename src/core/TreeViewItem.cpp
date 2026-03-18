#include <horizon/TreeViewItem.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Application.hpp>
#include <horizon/Spacer.hpp>

namespace horizon
{
    TreeViewItem::TreeViewItem(const std::string &icon_name, const std::string &text)
        : Widget(), m_icon_name(icon_name), m_text(text)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_spacing(0);
        set_margin(0);

        // Header row
        auto header = std::make_unique<Widget>();
        header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header->set_fixed_size(24);
        header->set_spacing(4);
        header->set_margin(0);
        m_header = header.get();

        // 1. Indentation
        auto spacer = std::make_unique<Widget>();
        spacer->set_fixed_size(0); 
        m_spacer = spacer.get();
        m_header->add_child(std::move(spacer));

        // 2. Disclosure (constant space)
        auto disc_container = std::make_unique<Widget>();
        disc_container->set_fixed_size(16);
        disc_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        disc_container->set_margin(0);
        m_disclosure_container = disc_container.get();

        auto disclosure = std::make_unique<Icon>();
        disclosure->set_icon_name("pan-end-symbolic");
        disclosure->set_icon_size(16);
        disclosure->set_fixed_size(16);
        disclosure->set_vertical_alignment(VerticalAlignment::Middle);
        m_disclosure_icon = disclosure.get();
        m_disclosure_icon->when_mouse_press.connect([this](MouseButtonEventContext &ev) {
            set_expanded(!m_expanded);
            ev.stop_propagation = true;
        });
        m_disclosure_container->add_child(std::move(disclosure));
        m_header->add_child(std::move(disc_container));

        // 3. Item Icon
        auto icon = std::make_unique<Icon>();
        icon->set_icon_name(m_icon_name);
        icon->set_icon_size(16);
        icon->set_fixed_size(18);
        icon->set_vertical_alignment(VerticalAlignment::Middle);
        m_item_icon = icon.get();
        m_header->add_child(std::move(icon));

        // 4. Label
        auto label = std::make_unique<Label>(m_text);
        label->set_font_size(12);
        label->set_alignment(TextAlignment::Left);
        label->set_vertical_alignment(VerticalAlignment::Middle);
        m_label = label.get();
        m_header->add_child(std::move(label));

        add_child(std::move(header));
    }

    void TreeViewItem::set_expanded(bool expanded)
    {
        if (m_expanded != expanded)
        {
            m_expanded = expanded;
            for (auto &child_ptr : children())
            {
                auto *item = dynamic_cast<TreeViewItem*>(child_ptr.get());
                if (item) item->set_visible(m_expanded);
            }
            
            if (m_disclosure_icon)
            {
                m_disclosure_icon->set_icon_name(m_expanded ? "pan-down-symbolic" : "pan-end-symbolic");
            }
            
            invalidate();
            // Upward invalidation is already handled by base Widget::invalidate()
        }
    }

    void TreeViewItem::set_bold(bool bold)
    {
        m_bold = bold;
        if (m_label) m_label->set_font_weight(m_bold ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);
        invalidate();
    }

    void TreeViewItem::add_item(std::unique_ptr<TreeViewItem> item)
    {
        item->m_indentation_level = m_indentation_level + 1;
        item->set_visible(m_expanded);
        add_child(std::move(item));
        invalidate();
    }

    void TreeViewItem::calculate_layout()
    {
        if (m_spacer) m_spacer->set_fixed_size(m_indentation_level * 16);

        bool has_subitems = false;
        for (const auto &child_ptr : children())
        {
            auto *item = dynamic_cast<TreeViewItem*>(child_ptr.get());
            if (item)
            {
                has_subitems = true;
                if (m_expanded && item->is_visible()) {
                    item->set_fixed_size(item->total_height());
                    // item->calculate_layout(); // Base Layout handles this
                }
            }
        }

        if (m_disclosure_icon) m_disclosure_icon->set_visible(has_subitems);

        // Root items in TreeView will have their fixed_size updated by TreeView
        Widget::calculate_layout();
    }

    void TreeViewItem::draw(GraphicsContext &gc)
    {
        if (is_hovered() || (m_header && m_header->is_hovered()))
        {
            gc.setColor(Color(0.9f, 0.9f, 0.9f, 0.3f));
            gc.fillRect(m_x, m_y, m_width, 24);
        }
    }

    Widget *TreeViewItem::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled) return nullptr;
        return Widget::hit_test(x, y);
    }

    int TreeViewItem::total_height() const
    {
        if (!is_visible()) return 0;
        int h = 24;
        if (m_expanded)
        {
            for (const auto &child_ptr : children())
            {
                auto *item = dynamic_cast<TreeViewItem*>(child_ptr.get());
                if (item && item->is_visible()) h += item->total_height();
            }
        }
        return h;
    }
} // namespace horizon
