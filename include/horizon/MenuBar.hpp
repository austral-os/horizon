#pragma once

#include <horizon/EventsManager.hpp>
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

        // Callback when a menu title is clicked: receives MenuBarClickContext
        EventsManager<MenuBarClickContext> when_menu_click;
        void set_menu_open(bool open);
        bool menu_open() const
        {
            return m_menu_open;
        }

    private:
        void update_selection(MenuBarItem *selected_item);

        std::vector<std::unique_ptr<Menu>> m_menus;
        bool m_menu_open = false;
    };

    class MenuBarItem : public Label
    {
    public:
        MenuBarItem(const std::string &title, Menu *menu);
        ~MenuBarItem() = default;

        void set_bold(bool bold)
        {
            m_bold = bold;
        }
        bool bold() const
        {
            return m_bold;
        }

        void set_icon_name(const std::string &name);
        const std::string &icon_name() const
        {
            return m_icon_name;
        }

        const std::string &resolved_icon_path() const
        {
            return m_resolved_icon_path;
        }

        void set_selected(bool selected);
        bool is_selected() const
        {
            return m_selected;
        }

        void draw(GraphicsContext &gc) override;
        int preferred_width() const override;

        Menu *menu() const
        {
            return m_menu;
        }

    private:
        bool m_selected = false;
        Menu *m_menu;
        bool m_bold = false;
        std::string m_icon_name;
        std::string m_resolved_icon_path;
    };
} // namespace horizon
