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
}

namespace horizon::arkfm
{
    struct MountPasswordEvent : public EventContext
    {
        storage::RemoteCredentials credentials;
    };

    class MountPasswordDialog : public WaylandWindow
    {
    public:
        MountPasswordDialog(const std::string& server_name);
        ~MountPasswordDialog() override = default;
        
        EventsManager<MountPasswordEvent> when_accepted;

    private:
        void update_enabled_state();
        
        RadioButton<AquaObject>* m_guest_radio{nullptr};
        RadioButton<AquaObject>* m_user_radio{nullptr};
        TextBox<TextPolicy>* m_name_input{nullptr};
        TextBox<PasswordPolicy>* m_pass_input{nullptr};
        Checkbox<AquaObject>* m_remember_check{nullptr};
    };
}
