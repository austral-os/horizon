#pragma once
#include <horizon/EventsManager.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/MenuSeparator.hpp>
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{
    struct SubmenuOpenEvent
    {
        MenuItem *item;      // item that owns the submenu
        Menu *submenu;       // submenu to open
        int item_x;          // item x in popup-local coords
        int item_y;          // item y in popup-local coords
        int item_w;          // item width
        int item_h;          // item height
        Menu *parent_menu;   // menu that owns the item
    };

    class Menu : public Widget
    {
    public:
        Menu();
        ~Menu() = default;

        void add_item(std::unique_ptr<MenuItem> item);
        void add_separator();

        void render(GraphicsContext &ctx, int cx, int cy, int cw, int ch,
                    bool force = false) override;

        void set_title(const std::string &title)
        {
            m_title = title;
        }
        const std::string &title() const
        {
            return m_title;
        }

        void set_bold(bool bold)
        {
            m_bold = bold;
        }
        bool bold() const
        {
            return m_bold;
        }

        void set_icon_name(const std::string &name)
        {
            m_icon_name = name;
        }
        const std::string &icon_name() const
        {
            return m_icon_name;
        }

        void set_icon_theme_color_key(const std::string &key)
        {
            m_icon_theme_color_key = key;
        }
        const std::string &icon_theme_color_key() const
        {
            return m_icon_theme_color_key;
        }

        // Helper to add a text item directly
        MenuItem *add_item(const std::string &text, const std::string &shortcut = "",
                           const std::string &item_id = "");

        void calculate_layout() override;
        Widget *hit_test(int x, int y) override;

        void set_max_width(int max_width)
        {
            m_max_width = max_width;
        }
        int max_width() const
        {
            return m_max_width;
        }
        
        void set_bottom_alpha(float alpha)
        {
            m_bottom_alpha = alpha;
        }

        void set_max_menu_height(int max_height)
        {
            m_max_menu_height = max_height;
        }

        double scroll_y() const
        {
            return m_scroll_y;
        }

        void set_min_width(int min_width)
        {
            m_min_width = min_width;
        }
        int min_width() const
        {
            return m_min_width;
        }

        void set_id(const std::string &id)
        {
            m_id = id;
        }
        const std::string &id() const
        {
            return m_id;
        }

        // Event emitted when a submenu needs to open as a native Wayland popup.
        // Connected by WaylandWindow to create a child xdg_popup.
        EventsManager<SubmenuOpenEvent> when_submenu_open;

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        int m_item_height = 24;
        int m_min_width = 240;
        int m_max_width = -1;
        double m_scroll_y = 0;
        int m_max_menu_height = 500;
        double m_total_content_height = 0;
        bool m_was_visible = false;
        std::string m_title;
        std::string m_id;
        bool m_bold = false;
        std::string m_icon_name;
        std::string m_icon_theme_color_key{"icon_fg"};
        float m_bottom_alpha = 0.8f;
    };

} // namespace horizon
