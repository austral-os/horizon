#pragma once

#include <functional>
#include <horizon/Label.hpp>
#include <horizon/Menu.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <vector>

namespace horizon
{
    class MenuBarItem;

    class MenuBar : public Widget
    {
    public:
        MenuBar();
        ~MenuBar() = default;

        void add_menu(std::unique_ptr<Menu> menu);
        void insert_menu(int index, std::unique_ptr<Menu> menu);
        void remove_menu(int index);
        void clear_menus();

        void calculate_layout() override;

        // Callback when a menu title is clicked: receives (Menu*, x, y) of the item
        void set_on_menu_click(std::function<void(Menu *, int x, int y)> callback);
        void set_menu_open(bool open);
        bool menu_open() const
        {
            return m_menu_open;
        }

    private:
        void update_selection(MenuBarItem *selected_item);

        std::vector<std::unique_ptr<Menu>> m_menus;
        std::function<void(Menu *, int x, int y)> m_on_menu_click;
        bool m_menu_open = false;
    };

    class MenuBarItem : public Label
    {
    public:
        MenuBarItem(const std::string &title, Menu *menu);
        ~MenuBarItem() = default;

        void set_selected(bool selected);
        bool is_selected() const
        {
            return m_selected;
        }

        void draw(GraphicsContext &gc) override;

        Menu *menu() const
        {
            return m_menu;
        }

    private:
        bool m_selected = false;
        Menu *m_menu;
    };
} // namespace horizon
