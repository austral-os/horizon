#pragma once
#include "horizon/WaylandWindow.hpp"
#include "horizon/EventsManager.hpp"
#include <string>

namespace horizon::arkfm
{
    struct RenameEvent : public EventContext
    {
        std::string new_name;
    };

    class RenameDialog : public WaylandWindow
    {
    public:
        RenameDialog(const std::string &current_name);
        ~RenameDialog() override = default;

        EventsManager<RenameEvent> when_accepted;
        EventsManager<EventContext> when_cancelled;

    private:
        void setup_ui();
        std::string m_current_name;
    };
} // namespace horizon::arkfm
