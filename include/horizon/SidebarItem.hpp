#pragma once
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    /**
     * @class SidebarItem
     * @brief A standard sidebar item with an icon and a text label.
     */
    class SidebarItem : public Widget
    {
    public:
        SidebarItem(const std::string &icon_name, const std::string &text);
        ~SidebarItem() = default;

        void draw(GraphicsContext &gc) override;
        void calculate_layout() override;
        Widget *hit_test(int x, int y) override;

        void set_selected(bool selected)
        {
            m_selected = selected;
            invalidate();
        }
        bool is_selected() const
        {
            return m_selected;
        }

    private:
        Icon *m_icon_ptr{nullptr};
        Label *m_label_ptr{nullptr};
        bool m_selected{false};
    };
} // namespace horizon
