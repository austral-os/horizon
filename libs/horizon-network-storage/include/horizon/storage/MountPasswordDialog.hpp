#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/storage/RemoteManager.hpp>
#include <string>

namespace horizon
{
    class TextBoxBase;
    template <typename Policy> class TextBox;
    struct TextPolicy;
    struct PasswordPolicy;
    template <typename T> class RadioButton;
    template <typename T> class Checkbox;
    class AquaObject;
    class Label;
    class ProgressBar;
}

namespace horizon::storage
{
    struct MountPasswordEvent : public EventContext
    {
        RemoteCredentials credentials;
    };

    class MountPasswordDialog : public WaylandWindow
    {
    public:
        MountPasswordDialog(const std::string& server_name);
        ~MountPasswordDialog() override = default;
        
        EventsManager<MountPasswordEvent> when_accepted;

        /**
         * @brief Shows the loading bar and disables inputs.
         */
        void show_loading();

        /**
         * @brief Shows an error message and stops loading.
         */
        void show_error(const std::string& message);

        /**
         * @brief Pre-fills the dialog with existing credentials.
         */
        void set_initial_credentials(const RemoteCredentials& creds);

    private:
        void update_enabled_state();
        
        RadioButton<AquaObject>* m_guest_radio{nullptr};
        RadioButton<AquaObject>* m_user_radio{nullptr};
        TextBox<TextPolicy>* m_name_input{nullptr};
        TextBox<PasswordPolicy>* m_pass_input{nullptr};
        Checkbox<AquaObject>* m_remember_check{nullptr};
        
        Label* m_error_label{nullptr};
        ProgressBar* m_loading_bar{nullptr};
        Widget* m_connect_btn{nullptr};
    };
}
