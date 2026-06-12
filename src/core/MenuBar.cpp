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
                
                m_dismiss_subscription = win->when_popup_dismissed.connect([this](PopupDismissedContext &ctx) { 
                    LOG_INFO << "[MenuBar] Popup dismissed signal received (serial=" << ctx.serial << "). Closing menu state.";
                    m_last_dismiss_serial = ctx.serial;

                    m_last_selected_item_before_dismiss = nullptr;
                    for (const auto &child : children())
                    {
                        auto *item = dynamic_cast<MenuBarItem *>(child.get());
                        if (item && item->is_selected())
                        {
                            m_last_selected_item_before_dismiss = item;
                            break;
                        }
                    }

                    set_menu_open(false); 
                });
            }
        });
    }

    void MenuBar::add_menu(std::unique_ptr<Menu> menu)
    {
        auto item = std::make_unique<MenuBarItem>(menu->title(), menu.get());
        item->set_bold(menu->bold());
        item->set_icon_theme_color_key(menu->icon_theme_color_key());
        item->set_icon_name(menu->icon_name());
        item->set_position_type(FREE);
        
        item->when_mouse_press.connect(
            [this, item_ptr = item.get()](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 0x110) // Left click
                {
                    ctx.stop_propagation = true;
                    update_selection(item_ptr, true, ctx.serial);
                }
            });
        item->when_mouse_enter.connect(
            [this, item_ptr = item.get()](EventContext &)
            {
                if (m_menu_open)
                {
                    // Wayland spurious 0,0 event workaround:
                    // If the pointer is not actually over this item, ignore the enter event.
                    auto pos = item_ptr->get_absolute_position();
                    auto *win = item_ptr->application();
                    if (win) {
                        int px = (int)win->pointer_x();
                        int py = (int)win->pointer_y();
                        LOG_INFO << "[MenuBar] Checking MouseEnter on " << item_ptr->text() << " (pointer at " << px << "," << py << ") bounds: " << pos.x << "," << pos.y << " to " << pos.x + item_ptr->width() << "," << pos.y + item_ptr->height();
                        if (px < pos.x || px > pos.x + item_ptr->width() || py < pos.y || py > pos.y + item_ptr->height())
                        {
                            LOG_INFO << "[MenuBar] Ignoring spurious MouseEnter on " << item_ptr->text() << " (pointer at " << px << "," << py << ")";
                            return;
                        }
                    } else {
                        LOG_INFO << "[MenuBar] MouseEnter on " << item_ptr->text() << " but application() is null!";
                    }

                    update_selection(item_ptr, false);
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
        item->set_icon_theme_color_key(menu->icon_theme_color_key());
        item->set_icon_name(menu->icon_name());
        item->set_position_type(FREE);
        
        item->when_mouse_press.connect(
            [this, item_ptr = item.get()](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 0x110) // Left click
                {
                    update_selection(item_ptr, true, ctx.serial);
                }
            });
        item->when_mouse_enter.connect(
            [this, item_ptr = item.get()](EventContext &)
            {
                if (m_menu_open)
                {
                    auto pos = item_ptr->get_absolute_position();
                    auto *win = item_ptr->application();
                    if (win) {
                        int px = (int)win->pointer_x();
                        int py = (int)win->pointer_y();
                        LOG_INFO << "[MenuBar] Checking MouseEnter on " << item_ptr->text() << " (pointer at " << px << "," << py << ") bounds: " << pos.x << "," << pos.y << " to " << pos.x + item_ptr->width() << "," << pos.y + item_ptr->height();
                        if (px < pos.x || px > pos.x + item_ptr->width() || py < pos.y || py > pos.y + item_ptr->height())
                        {
                            LOG_INFO << "[MenuBar] Ignoring spurious MouseEnter on " << item_ptr->text() << " (pointer at " << px << "," << py << ")";
                            return;
                        }
                    } else {
                        LOG_INFO << "[MenuBar] MouseEnter on " << item_ptr->text() << " but application() is null!";
                    }

                    update_selection(item_ptr, false);
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

    void MenuBar::update_selection(MenuBarItem *selected_item, bool is_explicit_click, uint32_t serial)
    {
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

        // --- THE FIX ---
        // If we just dismissed a menu with the SAME serial as this press, it means 
        // the press was ALREADY used to dismiss the previous menu by WaylandWindow.
        // In that case, we should NOT open a new one IF it was the same item.
        if (is_explicit_click && serial > 0 && serial == m_last_dismiss_serial && selected_item == m_last_selected_item_before_dismiss)
        {
            LOG_INFO << "[MenuBar] update_selection: Ignoring press because it was already used to dismiss (serial=" << serial << ", item=" << selected_item->text() << ")";
            return;
        }

        // Reset the tracker after we've used it (or bypassed it)
        m_last_selected_item_before_dismiss = nullptr;

        // Toggle logic: If clicking the already selected item, close it
        if (is_explicit_click && selected_item && selected_item == current_selected && m_menu_open)
        {
            LOG_INFO << "[MenuBar] update_selection: Toggling off " << selected_item->text();
            if (auto *win = dynamic_cast<WaylandWindow *>(application()))
            {
                win->close_context_menu();
            }
            return;
        }

        // If it's a mouse enter and the item is already selected, do nothing
        if (!is_explicit_click && selected_item && selected_item == current_selected && m_menu_open)
        {
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
            auto pos = selected_item->get_absolute_position();
            if (auto app = application()) {
                pos.x -= app->screen_x();
                pos.y -= app->screen_y();
            }
            ctx.x = pos.x;
            ctx.y = pos.y + selected_item->height();
            ctx.serial = serial;
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
        m_icon = nullptr;
        set_cursor_type(CursorType::Default);
    }

    void MenuBarItem::set_icon_theme_color_key(const std::string &key)
    {
        if (m_icon_theme_color_key == key)
            return;

        m_icon_theme_color_key = key;
        if (m_icon)
        {
            m_icon->set_theme_color_key(m_icon_theme_color_key);
        }
    }

    void MenuBarItem::set_icon_name(const std::string &name)
    {
        if (m_icon_name == name)
            return;

        m_icon_name = name;
        if (!m_icon_name.empty())
        {
            if (!m_icon)
            {
                auto icon = std::make_unique<Icon>();
                m_icon = icon.get();
                m_icon->set_position_type(FREE);
                m_icon->set_use_theme_colors(true);
                m_icon->set_theme_color_key(m_icon_theme_color_key);
                add_child(std::move(icon));
            }
            m_icon->set_icon_name(m_icon_name);
            m_icon->set_icon_size(18);
            m_icon->set_visible(true);
        }
        else if (m_icon)
        {
            m_icon->set_visible(false);
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
        if (m_selected == selected)
            return;

        m_selected = selected;
        if (m_icon)
        {
            if (m_selected)
            {
                m_icon->set_use_theme_colors(false);
                m_icon->set_icon_color({1.0f, 1.0f, 1.0f, 1.0f});
            }
            else
            {
                m_icon->set_use_theme_colors(true);
            }
        }
        invalidate();
    }

    void MenuBarItem::calculate_layout()
    {
        Widget::calculate_layout();

        if (m_icon && !m_icon_name.empty() && m_icon->is_visible())
        {
            int icon_size = 18;
            int total_content_width = icon_size;
            if (!text().empty())
            {
                total_content_width += 5 + Label::preferred_width();
            }

            int start_x = (m_width - total_content_width) / 2;
            int icon_y = (m_height - icon_size) / 2;

            m_icon->set_position(m_start_draw_x + start_x, m_start_draw_y + icon_y);
            m_icon->set_size(icon_size, icon_size);
            m_icon->calculate_layout();
        }
    }

    void MenuBarItem::draw(GraphicsContext &gc)
    {
        if (m_selected)
        {
            // Selected: Blueish background, white text
            gc.setColor(Color(0.2f, 0.45f, 0.9f, 1.0f));
            gc.fillRect(m_start_draw_x, m_start_draw_y, m_width, m_height);
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

        // Draw children (Icon)
        Widget::draw(gc);

        if (!m_icon_name.empty() && !text().empty())
        {
            int icon_size = 18;
            int total_content_width = icon_size + 5 + Label::preferred_width();

            int start_x = (m_width - total_content_width) / 2;

            gc.setDrawFont(nullptr, font_size() > 0 ? font_size() : 13, FONT_SLANT_NORMAL,
                           m_bold ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

            TextMetrics tm = gc.getTextMetrics(
                text().c_str(), nullptr, font_size() > 0 ? font_size() : 13, FONT_SLANT_NORMAL,
                m_bold ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL);

            int text_x = m_start_draw_x + start_x + icon_size + 5;
            int text_y = m_start_draw_y + (m_height + tm.height) / 2 - 2; // -2 for baseline adjustment

            gc.drawText(text_x, text_y, text().c_str());
        }
        else if (m_icon_name.empty())
        {
            Label::draw(gc);
        }
    }

} // namespace horizon
