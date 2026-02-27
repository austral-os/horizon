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
         * @brief Set desired icon size in pixels (e.g. 16, 24, 32, 48).
         * Triggers icon re-resolution and repaint.
         */
        void set_icon_size(int size);
        int icon_size() const;

        /**
         * @brief Get the resolved file path (for debugging/info).
         */
        const std::string &resolved_path() const;

    protected:
        void draw(GraphicsContext &ctx) override;

    private:
        std::string m_icon_name;
        int m_icon_size{24};
        std::string m_resolved_path;

        void resolve_icon();
    };

} // namespace horizon
