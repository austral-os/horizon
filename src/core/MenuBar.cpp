#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>
#include <horizon/MenuBar.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/IconThemeLookup.hpp>

namespace horizon
{
    MenuBar::MenuBar()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);

        when_application_load.connect([this](EventContext&) {
            if (auto *win = dynamic_cast<WaylandWindow *>(application()))
            {
                if (m_dismiss_subscription != 0) {
                    win->when_popup_dismissed.disconnect(m_dismiss_subscription);
                }
                
                m_dismiss_subscription = win->when_popup_dismissed.connect([this](PopupDismissedContext &) { 
                    LOG_INFO << "[MenuBar] Popup dismissed signal received. Closing menu state.";
                    set_menu_open(false); 
                });
            }
        });
    }

    void MenuBar::add_menu(std::unique_ptr<Menu> menu)
    {
        auto item = std::make_unique<MenuBarItem>(menu->title(), menu.get());
        item->set_bold(menu->bold());
        item->set_icon_name(menu->icon_name());
        item->set_position_type(FREE);
        
        item->when_mouse_press.connect(
            [this, item_ptr = item.get()](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 0x110) // Left click
                {
                    update_selection(item_ptr);
                }
            });
        item->when_mouse_enter.connect(
            [this, item_ptr = item.get()](EventContext &)
            {
                if (m_menu_open)
                {
                    update_selection(item_ptr);
                }
            });

        m_menus.push_back(std::move(menu));
        add_child(std::move(item));
        invalidate();
    }

    void MenuBar::insert_menu(int index, std::unique_ptr<Menu> menu)
    {
        auto item = std::make_unique<MenuBarItem>(menu->title(), menu.get());
        item->set_bold(menu->bold());
        item->set_icon_name(menu->icon_name());
        item->set_position_type(FREE);
        
        item->when_mouse_press.connect(
            [this, item_ptr = item.get()](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 0x110) // Left click
                {
                    update_selection(item_ptr);
                }
            });
        item->when_mouse_enter.connect(
            [this, item_ptr = item.get()](EventContext &)
            {
                if (m_menu_open)
                {
                    update_selection(item_ptr);
                }
            });

        m_menus.insert(m_menus.begin() + index, std::move(menu));
        add_child_at(index, std::move(item));
        invalidate();
    }

    void MenuBar::remove_menu(int index)
    {
        if (index >= 0 && index < (int)m_menus.size())
        {
            m_menus.erase(m_menus.begin() + index);
            remove_child_at(index);
        }
    }

    void MenuBar::clear_menus()
    {
        LOG_INFO << "[MenuBar] Clearing all menus.";
        auto *win = dynamic_cast<WaylandWindow *>(application());
        if (win)
        {
            win->close_context_menu();
        }
        clear_children();
        m_menus.clear();
        invalidate();
    }

    void MenuBar::calculate_layout()
    {
        Widget::calculate_layout();

        int current_x = m_start_draw_x;
        int total_width = 0;
        for (auto &child : m_children)
        {
            if (!child->is_visible()) continue;

            // Use preferred width (content based) + 20px padding
            int child_width = child->preferred_width() + 20;

            child->set_position(current_x, m_start_draw_y);
            child->set_size(child_width, m_available_draw_height);
            int step = child_width + m_spacing;
            current_x += step;
            total_width += step;
        }

        // Subtract the last spacing if any children
        if (!m_children.empty() && total_width > m_spacing) {
            total_width -= m_spacing;
        }

        // update our own size information
        int final_width = total_width + (m_margin * 2);
        
        // We use m_fixed_size directly to avoid recursion with set_fixed_size() 
        // calling invalidate() while we are already in layout/render.
        if (m_fixed_size != final_width) {
            m_fixed_size = final_width;
        }
    }

    void MenuBar::update_selection(MenuBarItem *selected_item)
    {
        LOG_INFO << "[MenuBar] Updating selection. Selected item: " 
                 << (selected_item ? selected_item->text() : "NULL")
                 << ", Current menu open state: " << m_menu_open;

        MenuBarItem* current_selected = nullptr;
        for (const auto &child : children())
        {
            auto *item = dynamic_cast<MenuBarItem *>(child.get());
            if (item && item->is_selected())
            {
                current_selected = item;
                break;
            }
        }

        // Toggle logic: If clicking the already selected item, close it
        if (selected_item && selected_item == current_selected && m_menu_open)
        {
            if (auto *win = dynamic_cast<WaylandWindow *>(application()))
            {
                win->close_context_menu();
            }
            return;
        }

        for (const auto &child : children())
        {
            auto *item = dynamic_cast<MenuBarItem *>(child.get());
            if (item)
            {
                item->set_selected(item == selected_item);
            }
        }

        if (selected_item)
        {
            m_menu_open = true;

            MenuBarClickContext ctx;
            ctx.sender = this;
            ctx.menu = selected_item->menu();
            ctx.x = selected_item->x();
            ctx.y = selected_item->y() + selected_item->height();
            when_menu_click.run(ctx);
        }
    }

    void MenuBar::set_menu_open(bool open)
    {
        m_menu_open = open;
        if (!open)
        {
            update_selection(nullptr);
        }
    }

    // --- MenuBarItem ---

    MenuBarItem::MenuBarItem(const std::string &title, Menu *menu) : Label(title), m_menu(menu)
    {
        set_alignment(TextAlignment::Center);
    }

    void MenuBarItem::set_icon_name(const std::string &name)
    {
        if (m_icon_name == name)
            return;

        m_icon_name = name;
        if (!m_icon_name.empty())
        {
            m_resolved_icon_path = IconThemeLookup::find_icon(m_icon_name, 18);
            if (m_resolved_icon_path.empty())
            {
                LOG_ERROR << "[MenuBarItem] Failed to resolve icon: " << m_icon_name;
            }
            else
            {
                LOG_INFO << "[MenuBarItem] Resolved icon " << m_icon_name << " to "
                         << m_resolved_icon_path;
            }
        }
        else
        {
            m_resolved_icon_path.clear();
        }
        invalidate();
    }

    int MenuBarItem::preferred_width() const
    {
        if (!m_icon_name.empty() && text().empty())
        {
            return 24; // Standard icon size in menu bar
        }

        int width = Label::preferred_width();
        if (!m_icon_name.empty())
        {
            width += 24 + 5; // Icon size + spacing
        }
        return width;
    }

    void MenuBarItem::set_selected(bool selected)
    {
        m_selected = selected;
        invalidate();
    }

    void MenuBarItem::draw(GraphicsContext &gc)
    {
        if (m_selected)
        {
            // Selected: Blueish background, white text
            gc.setColor(Color(0.2f, 0.45f, 0.9f, 1.0f));
            gc.fillRect(m_x, m_y, m_width, m_height);
            set_text_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
        }
        else
        {
            // Not selected: transparent background, default text color
            set_text_color(Color(0.0f, 0.0f, 0.0f, -1.0f)); // Use theme default
        }

        if (m_bold)
        {
            set_font_weight(FONT_WEIGHT_BOLD);
        }
        else
        {
            set_font_weight(FONT_WEIGHT_NORMAL);
        }

        if (!m_icon_name.empty())
        {
            int icon_size = 18;
            int total_content_width = icon_size;
            if (!text().empty())
            {
                total_content_width += 5 + Label::preferred_width();
            }

            int start_x = m_x + (m_width - total_content_width) / 2;
            int icon_y = m_y + (m_height - icon_size) / 2;

            if (!m_resolved_icon_path.empty())
            {
                gc.drawImage(m_resolved_icon_path, start_x, icon_y, icon_size, icon_size);
            }

            if (!text().empty())
            {
                gc.setDrawFont(nullptr, font_size() > 0 ? font_size() : 13, FONT_SLANT_NORMAL,
                               m_bold ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

                TextMetrics tm = gc.getTextMetrics(
                    text().c_str(), nullptr, font_size() > 0 ? font_size() : 13, FONT_SLANT_NORMAL,
                    m_bold ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

                int text_x = start_x + icon_size + 5;
                int text_y = m_y + (m_height + tm.height) / 2 - 2; // -2 for baseline adjustment

                gc.drawText(text_x, text_y, text().c_str());
            }
        }
        else
        {
            Label::draw(gc);
        }
    }

} // namespace horizon
