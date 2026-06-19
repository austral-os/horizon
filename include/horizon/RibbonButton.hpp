#pragma once

#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{

    enum class RibbonButtonTextPosition
    {
        BelowIcon,
        RightOfIcon
    };

    enum class RibbonButtonSize
    {
        XLarge,
        Large,
        Normal,
        Small,
        XSmall
    };

    class RibbonButton : public Widget
    {
    public:
        RibbonButton();
        ~RibbonButton() override = default;

        void set_icon(const std::string &icon_name);
        void set_text(const std::string &text);

        void set_text_position(RibbonButtonTextPosition position);
        void set_button_size(RibbonButtonSize size);

        void set_active(bool active);
        bool is_active() const;

        void set_font_size(int size);
        int font_size() const;

        void set_font_weight(FontWeight weight);
        FontWeight font_weight() const;

        int preferred_width() const override;
        int preferred_height() const override;
        int preferred_height(int width) const override;

        void calculate_layout() override;

        void set_application_recursive(WaylandWindow *app) override;

    protected:
        void draw(GraphicsContext &ctx) override;

    private:
        std::unique_ptr<SolidObject> m_background;
        std::unique_ptr<Icon> m_icon;
        std::unique_ptr<Label> m_label;

        RibbonButtonTextPosition m_text_position{RibbonButtonTextPosition::BelowIcon};
        RibbonButtonSize m_button_size{RibbonButtonSize::Normal};

        bool m_hovered = false;
        bool m_active = false;

        void update_icon_size();
    };

} // namespace horizon
