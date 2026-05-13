#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/EventsManager.hpp>
#include <string>

namespace horizon
{
    class Combo;
}

namespace horizon::keyring
{
    struct ItemEvent : public EventContext
    {
        std::string label;
        std::string secret;
        std::string type;
    };

    class ItemDialog : public WaylandWindow
    {
    public:
        ItemDialog(const std::string& title);
        ~ItemDialog() override = default;

        EventsManager<ItemEvent> when_accepted;
        EventsManager<EventContext> when_cancelled;

        void set_initial_values(const std::string& label, const std::string& secret, const std::string& type);

    private:
        void setup_ui();

        TextBox<TextPolicy>* m_txt_label{nullptr};
        TextBox<PasswordPolicy>* m_txt_secret{nullptr};
        horizon::Combo* m_cmb_type{nullptr};
    };
}
