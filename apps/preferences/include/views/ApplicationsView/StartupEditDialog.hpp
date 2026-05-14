#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/DesktopManager.hpp>
#include <string>

namespace horizon::preferences
{
    class StartupEditDialog : public WaylandWindow
    {
    public:
        StartupEditDialog(const horizon::DesktopEntry& entry);
        ~StartupEditDialog() override = default;

        EventsManager<std::string> when_accepted;
        EventsManager<EventContext> when_cancelled;

    private:
        void setup_ui(const horizon::DesktopEntry& entry);

        TextBox<TextPolicy>* m_command_input{nullptr};
    };
} // namespace horizon::preferences
