#include <horizon/Application.hpp>
#include <horizon/MenuBar.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    MenuBar::MenuBar()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);
    }

    void MenuBar::add_menu(std::unique_ptr<Menu> menu)
    {
        auto item = std::make_unique<MenuBarItem>(menu->title(), menu.get());
        item->set_position_type(FREE);
        item->when_mouse_press.connect(
            [this, item_ptr = item.get()](EventContext &ctx)
            {
                if (ctx.button == 0x110) // Left click
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
        item->set_position_type(FREE);
        item->when_mouse_press.connect(
            [this, item_ptr = item.get()](EventContext &ctx)
            {
                if (ctx.button == 0x110) // Left click
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
        m_menus.clear();
        clear_children();
        invalidate();
    }

    void MenuBar::calculate_layout()
    {
        Widget::calculate_layout();

        int current_x = m_start_draw_x;
        for (auto &child : m_children)
        {
            int child_width = child->width() > 0 ? child->width() : 80;
            child->set_position(current_x, m_start_draw_y);
            child->set_size(child_width, m_available_draw_height);
            current_x += child_width + m_spacing;
        }
    }

    void MenuBar::update_selection(MenuBarItem *selected_item)
    {
        for (const auto &child : children())
        {
            auto *item = dynamic_cast<MenuBarItem *>(child.get());
            if (item)
            {
                item->set_selected(item == selected_item);
            }
        }

        if (m_on_menu_click && selected_item)
        {
            // Pass the menu and the bottom-left corner of the clicked item
            m_on_menu_click(selected_item->menu(), selected_item->x(),
                            selected_item->y() + selected_item->height());
        }
    }

    void MenuBar::set_on_menu_click(std::function<void(Menu *, int x, int y)> callback)
    {
        m_on_menu_click = std::move(callback);
    }

    // --- MenuBarItem ---

    MenuBarItem::MenuBarItem(const std::string &title, Menu *menu) : Label(title), m_menu(menu)
    {
        set_alignment(TextAlignment::Center);
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

        Label::draw(gc);
    }

} // namespace horizon
