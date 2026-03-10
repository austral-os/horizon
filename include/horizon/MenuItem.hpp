#pragma once
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{
    class Menu;

    class MenuItem : public Widget
    {
    public:
        MenuItem();
        explicit MenuItem(const std::string &text);
        virtual ~MenuItem() = default;

        void draw(GraphicsContext &gc) override;

        void set_text(const std::string &text);
        const std::string &text() const;

        void set_shortcut(const std::string &shortcut);
        const std::string &shortcut() const
        {
            return m_shortcut_text;
        }

        void set_icon(const std::string &icon_path);

        void set_id(const std::string &id)
        {
            m_id = id;
        }
        const std::string &id() const
        {
            return m_id;
        }

        void set_has_submenu(bool has_submenu);
        bool has_submenu() const
        {
            return m_has_submenu;
        }

        void set_selected(bool selected);
        bool is_selected() const
        {
            return m_selected;
        }

        // Allow setting a custom widget for the main content
        void set_content_widget(std::unique_ptr<Widget> widget);

        void set_submenu(Menu *submenu);
        Menu *submenu() const
        {
            return m_submenu;
        }

        // Returns the minimum width needed to display this item without truncation
        int preferred_width() const override;

        bool has_icon() const
        {
            return m_icon != nullptr;
        }
        void set_reserve_icon_space(bool reserve)
        {
            m_reserve_icon_space = reserve;
        }

    private:
        Icon *m_icon = nullptr;
        Widget *m_content = nullptr; // Usually a Label
        Label *m_shortcut_label = nullptr;

        std::string m_shortcut_text;
        Menu *m_submenu = nullptr;
        bool m_has_submenu = false;
        bool m_selected = false;
        bool m_reserve_icon_space = false;
        std::string m_id;

        void calculate_layout() override;
    };

} // namespace horizon
