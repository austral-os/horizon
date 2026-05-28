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
        ToolbarButton(const std::string &title, const std::string &icon_name, int icon_size = -1);
        ~ToolbarButton() override = default;

        void set_active(bool active);
        bool is_active() const { return m_active; }

        void set_title(const std::string &title);
        void set_icon_name(const std::string &icon_name);
        void set_icon_size(int size);
        int icon_size() const { return m_icon_size; }

        void set_text_color(Color color);

        void draw(GraphicsContext &gc) override;
        void calculate_layout() override;

    private:
        std::string m_title;
        std::string m_icon_name;
        int m_icon_size{-1};
        bool m_active{false};
        Color m_text_color{0.0f, 0.0f, 0.0f, -1.0f};
    };
} // namespace horizon
