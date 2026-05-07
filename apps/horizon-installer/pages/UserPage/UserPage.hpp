#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/Label.hpp>
#include <horizon/ToolbarButton.hpp>
#include <string>

namespace horizon { class AvatarSelector; }
 
namespace horizon::installer
{
    /**
     * @brief OOBE Page: User account configuration.
     */
    class UserPage : public Widget
    {
    public:
        UserPage();
        ~UserPage() override = default;

        std::string fullname() const;
        std::string username() const;
        std::string password() const;
        std::string avatar() const;

        EventsManager<EventContext> when_continue;
        EventsManager<EventContext> when_back;

    private:
        Widget* m_fullname_box{nullptr};
        Widget* m_username_box{nullptr};
        Widget* m_password_box{nullptr};
        Widget* m_verify_box{nullptr};
        Label* m_error_label{nullptr};
        ToolbarButton* m_next_btn{nullptr};
        horizon::AvatarSelector* m_avatar_selector{nullptr};

        void validate_inputs();
    };
} // namespace horizon::installer
