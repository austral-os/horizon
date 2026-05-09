#pragma once
#include "horizon/WaylandWindow.hpp"
#include "horizon/EventsManager.hpp"
#include <string>

namespace horizon::arkfm
{
    struct GoToFolderEvent : public EventContext
    {
        std::string path;
    };

    class GoToFolderDialog : public WaylandWindow
    {
    public:
        GoToFolderDialog();
        ~GoToFolderDialog() override = default;

        EventsManager<GoToFolderEvent> when_accepted;
        EventsManager<EventContext> when_cancelled;

    private:
        void setup_ui();
    };
} // namespace horizon::arkfm
