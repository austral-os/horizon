#pragma once

#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    /**
     * @brief A generic button for toolbars with an icon and a label.
     */
    class ToolbarButton : public Widget
    {
    public:
        ToolbarButton(std::string title, std::string icon_name);
        ~ToolbarButton() override = default;

        void set_active(bool active);
        bool is_active() const { return m_active; }

        void draw(GraphicsContext &gc) override;

    private:
        std::string m_title;
        std::string m_icon_name;
        bool m_active{false};
    };
} // namespace horizon
