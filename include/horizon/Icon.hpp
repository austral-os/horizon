#pragma once
#include <horizon/GraphicsContext.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{

    /**
     * @brief A widget that displays an icon from the active Linux icon theme.
     *
     * Usage:
     *   auto icon = std::make_unique<Icon>();
     *   icon->set_icon_name("folder");
     *   icon->set_icon_size(32);
     */
    class Icon : public Widget
    {
    public:
        Icon();
        ~Icon();

        /**
         * @brief Set the icon name (e.g. "folder", "firefox", "terminal").
         * Triggers icon resolution and repaint.
         */
        void set_icon_name(const std::string &name);
        const std::string &icon_name() const;

        /**
         * @brief Set a direct image path to render, bypassing theme lookup.
         * Clears any previously set icon name.
         */
        void set_icon_path(const std::string &path);

        /**
         * @brief Set desired icon size in pixels (e.g. 16, 24, 32, 48).
         * Triggers icon re-resolution and repaint.
         */
        void set_icon_size(int size);
        int icon_size() const;

        /**
         * @brief Get the resolved file path (for debugging/info).
         */
        const std::string &resolved_path() const;

        /**
         * @brief Set horizontal alignment of the icon.
         */
        void set_horizontal_alignment(TextAlignment alignment);
        TextAlignment horizontal_alignment() const;

        /**
         * @brief Set vertical alignment of the icon.
         */
        void set_vertical_alignment(VerticalAlignment alignment);
        VerticalAlignment vertical_alignment() const;

        /**
         * @brief Shadow Widget::set_fixed_size to ignore 0.
         */
        void set_fixed_size(int size);

        int preferred_width() const override;
        int preferred_height() const override;
        int preferred_height(int width) const override;

        /**
         * @brief Set the icon opacity (0.0 to 1.0).
         */
        void set_opacity(float opacity);
        float opacity() const;

        /**
         * @brief Set a color to tint the icon. If alpha is 0 (default), the icon is drawn normally.
         */
        void set_icon_color(Color color);
        Color icon_color() const;

        /**
         * @brief Enable or disable the use of theme-specific colors for the icon.
         * If enabled, the icon will be tinted with the "icon_fg" color from the theme.
         */
        void set_use_theme_colors(bool use);
        bool use_theme_colors() const;

    protected:
        void draw(GraphicsContext &ctx) override;

    private:
        std::string m_icon_name;
        int m_icon_size{24};
        VerticalAlignment m_vertical_alignment{VerticalAlignment::Middle};
        TextAlignment m_horizontal_alignment{TextAlignment::Center};
        std::string m_resolved_path;
        float m_opacity{1.0f};
        Color m_icon_color{0.0f, 0.0f, 0.0f, 0.0f};
        bool m_use_theme_colors{false};

        void resolve_icon();
    };

} // namespace horizon
