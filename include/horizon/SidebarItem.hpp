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

    private:
        Icon *m_icon_ptr{nullptr};
        Label *m_label_ptr{nullptr};
    };
} // namespace horizon
