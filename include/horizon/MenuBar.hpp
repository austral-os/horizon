#pragma once

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

    private:
        void update_selection(MenuBarItem *selected_item);

        std::vector<std::unique_ptr<Menu>> m_menus;
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
