#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <string>

namespace horizon
{
    enum class MessageType
    {
        Info,
        Warning,
        Error,
        Question
    };

    enum class MessageResponse
    {
        Accept,
        Cancel
    };

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

    private:
        void setup_ui(const std::string &message, MessageType type, bool show_cancel);
        std::string get_icon_for_type(MessageType type);
        WidgetAccentColor get_color_for_type(MessageType type);
    };
} // namespace horizon
