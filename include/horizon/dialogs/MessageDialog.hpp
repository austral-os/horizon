#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/DialogTypes.hpp>
#include <string>

namespace horizon
{
    class Label;
    template <typename T> class Button;
    class AquaObject;



    struct MessageResponseEvent : public EventContext
    {
        MessageResponse response;
    };

    class MessageDialog : public WaylandWindow
    {
    public:
        MessageDialog(const std::string &title, const std::string &message, MessageType type, bool show_cancel = false);
        ~MessageDialog() override = default;

        EventsManager<MessageResponseEvent> when_responded;

        void set_message(const std::string &message);
        void set_accept_text(const std::string &text);
        void set_cancel_text(const std::string &text);

    private:
        void setup_ui(const std::string &message, MessageType type, bool show_cancel);
        std::string get_icon_for_type(MessageType type);
        WidgetAccentColor get_color_for_type(MessageType type);

        Label *m_label{nullptr};
        Button<AquaObject> *m_accept_btn{nullptr};
        Button<AquaObject> *m_cancel_btn{nullptr};
    };
} // namespace horizon
