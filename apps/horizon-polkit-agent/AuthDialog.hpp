#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/EventsManager.hpp>
#include <string>

namespace horizon::polkit
{
    class AuthSuccessEvent : public EventContext
    {
    public:
        std::string password;
    };

    class AuthCancelEvent : public EventContext
    {
    };

    class AuthDialog : public WaylandWindow
    {
    public:
        AuthDialog(const std::string& action_id, const std::string& message, const std::string& user);
        virtual ~AuthDialog() = default;

        void setup_ui(const std::string& message, const std::string& user);
        void on_authenticate();
        
        // Muestra un mensaje de error en la UI
        void show_error(const std::string& error);
        
        // Limpia el campo de contraseña y le devuelve el foco
        void reset_password();

        EventsManager<AuthSuccessEvent> when_authenticated;
        EventsManager<AuthCancelEvent> when_canceled;

    private:
        TextBox<PasswordPolicy>* m_password_entry{nullptr};
        Label* m_message_label{nullptr};
        Label* m_error_label{nullptr};
    };
}
