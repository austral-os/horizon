#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/TextBox.hpp>
#include <string>

namespace horizon::preferences
{
    class InputDialog : public WaylandWindow
    {
    public:
        InputDialog(const std::string &title, const std::string &prompt);
        ~InputDialog() override = default;

        EventsManager<std::string> when_accepted;
        EventsManager<EventContext> when_cancelled;

    private:
        void setup_ui(const std::string &prompt);

        TextBox<TextPolicy> *m_input{nullptr};
    };
} // namespace horizon::preferences
