#pragma once
#include <horizon/EventsManager.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/MenuSeparator.hpp>
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{

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

        // Helper to add a text item directly
        MenuItem *add_item(const std::string &text, const std::string &shortcut = "",
                           const std::string &item_id = "");

        void calculate_layout() override;
        Widget *hit_test(int x, int y) override;

        void close_submenus();
        void set_active_submenu(Menu *menu);

        void set_max_width(int max_width)
        {
            m_max_width = max_width;
        }
        int max_width() const
        {
            return m_max_width;
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

        /**
         * @brief Calculates the total extra width needed for all nested submenus.
         * Used to size the popup surface so submenus don't get clipped.
         */
        int calculate_cascade_width() const;

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        Menu *m_active_submenu = nullptr;
        int m_item_height = 24;
        int m_min_width = 240;
        int m_max_width = -1;       // -1 means no maximum
        double m_scroll_y = 0;
        int m_max_menu_height = 500;
        double m_total_content_height = 0;
        bool m_was_visible = false; // Track visibility transitions for clearing
        std::string m_title;
        std::string m_id;
        bool m_bold = false;
        std::string m_icon_name;
    };

} // namespace horizon
