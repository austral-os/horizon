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
        ToolbarButton(const std::string &title, const std::string &icon_name);
        ~ToolbarButton() override = default;

        void set_active(bool active);
        bool is_active() const { return m_active; }

        void set_text_color(Color color);

        void draw(GraphicsContext &gc) override;

    private:
        std::string m_title;
        std::string m_icon_name;
        bool m_active{false};
        Color m_text_color{0.0f, 0.0f, 0.0f, -1.0f};
    };
} // namespace horizon
