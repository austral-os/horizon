#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <string>

namespace horizon::preferences
{
    struct PasswordDialogEvent : public EventContext
    {
        bool accepted;
        std::string password;
    };

    class PasswordDialog : public WaylandWindow
    {
    public:
        PasswordDialog(const std::string &username);
        ~PasswordDialog() override = default;

        EventsManager<PasswordDialogEvent> when_finished;

    private:
        void setup_ui();
        void on_accept();
        void on_cancel();

        std::string m_username;
        TextBox<PasswordPolicy> *m_pass1_box{nullptr};
        TextBox<PasswordPolicy> *m_pass2_box{nullptr};
        Label *m_error_label{nullptr};
    };
}
