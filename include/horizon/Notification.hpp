#pragma once

#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    /**
     * @brief A widget for displaying notifications with an optional icon and a message.
     */
    class Notification : public Widget
    {
    public:
        Notification();
        virtual ~Notification() = default;

        /**
         * @brief Sets the icon name (from the icon theme).
         */
        void set_icon_name(const std::string &name);

        /**
         * @brief Sets the message text.
         */
        void set_message(const std::string &message);

        /**
         * @brief Sets both the icon and the message.
         */
        void set_notification(const std::string &icon_name, const std::string &message);

        /**
         * @brief Sets a fixed width for the notification. If <= 0, width is calculated based on
         * content.
         */
        void set_fixed_width(int width);

        /**
         * @brief Returns the current message text.
         */
        const std::string &message() const;

        /**
         * @brief Returns the current icon name.
         */
        const std::string &icon_name() const;

        // Overrides for layout and rendering
        int preferred_width() const override;
        int preferred_height() const override;
        int preferred_height(int width) const override;

        void calculate_layout() override;

    protected:
        void draw(GraphicsContext &ctx) override;

    private:
        Icon *m_icon_widget{nullptr};
        Label *m_label_widget{nullptr};

        int m_internal_padding{15};
        int m_icon_size{32};
    };

} // namespace horizon
