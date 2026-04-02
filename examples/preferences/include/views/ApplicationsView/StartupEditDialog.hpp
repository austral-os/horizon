#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <utils/DesktopManager.hpp>
#include <string>

namespace horizon::preferences
{
    class StartupEditDialog : public WaylandWindow
    {
    public:
        StartupEditDialog(const DesktopEntry& entry);
        ~StartupEditDialog() override = default;

        EventsManager<std::string> when_accepted;
        EventsManager<EventContext> when_cancelled;

    private:
        void setup_ui(const DesktopEntry& entry);

        TextBox<TextPolicy>* m_command_input{nullptr};
    };
} // namespace horizon::preferences
