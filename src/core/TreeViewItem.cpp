#include <horizon/TreeViewItem.hpp>
#include <horizon/TreeView.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Application.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ThemeManager.hpp>

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

        // Interaction: click on the whole header to toggle expand or select
        m_header->when_mouse_press.connect([this](MouseButtonEventContext &ev) {
            if (has_children())
            {
                set_expanded(!m_expanded);
            }
            else
            {
                // Find TreeView parent to set selection
                Widget *p = parent();
                while (p)
                {
                    auto *tv = dynamic_cast<TreeView *>(p);
                    if (tv)
                    {
                        tv->set_selected_item(this);
                        break;
                    }
                    p = p->parent();
                }
            }
            ev.stop_propagation = true;
        });
    }

    void TreeViewItem::set_selected(bool selected)
    {
        if (m_selected != selected)
        {
            m_selected = selected;
            
            // Update text color for label and disclosure icon
            if (m_selected) {
                if (auto *app = application()) {
                    auto *tm = app->theme_manager.get();
                    Color fg = tm->get_color("table_row_selected_fg");
                    if (m_label) m_label->set_text_color(fg);
                    if (m_disclosure_icon) m_disclosure_icon->set_icon_color(fg);
                }
            } else {
                Color fg = Color(0.0f, 0.0f, 0.0f, 1.0f); // Default fallback
                if (auto *app = application()) {
                    if (auto *tm = app->theme_manager.get()) {
                        fg = tm->get_color("window_fg");
                    }
                }
                if (m_label) m_label->set_text_color(fg);
                if (m_disclosure_icon) m_disclosure_icon->set_icon_color(fg);
            }
            
            invalidate();
        }
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
            
            // Layout must be recalculated from the tree view down
            Widget *p = parent();
            while (p) {
                p->calculate_layout();
                p = p->parent();
            }
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
        int current_total_height = 24;

        for (const auto &child_ptr : children())
        {
            auto *item = dynamic_cast<TreeViewItem*>(child_ptr.get());
            if (item)
            {
                has_subitems = true;
                item->set_visible(m_expanded);
                if (m_expanded) {
                    item->set_fixed_size(item->total_height());
                    current_total_height += item->total_height();
                }
            }
        }

        if (m_disclosure_icon) m_disclosure_icon->set_visible(has_subitems);
        
        // Ensure this widget's height reflects its expanded state
        set_height(current_total_height);

        Widget::calculate_layout();
    }

    void TreeViewItem::draw(GraphicsContext &gc)
    {
        if (m_selected)
        {
            // Blue gradient selection (macOS Aqua style)
            Color c1(0.32f, 0.61f, 0.90f, 1.0f); // Top
            Color c2(0.11f, 0.45f, 0.81f, 1.0f); // Bottom
            gc.fillLinearGradientRect(m_start_draw_x, m_start_draw_y, m_available_draw_width, 24, c1, c2, true);
        }
    }

    Widget *TreeViewItem::hit_test(int x, int y)
    {
        if (!m_visible || !m_enabled) return nullptr;
        return Widget::hit_test(x, y);
    }

    void TreeViewItem::set_application_recursive(WaylandWindow *app)
    {
        Widget::set_application_recursive(app);
        if (!m_selected) {
            Color fg = Color(0.0f, 0.0f, 0.0f, 1.0f);
            if (app && app->theme_manager) {
                fg = app->theme_manager->get_color("window_fg");
            }
            if (m_label) m_label->set_text_color(fg);
            if (m_disclosure_icon) m_disclosure_icon->set_icon_color(fg);
        }
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
