#pragma once
#include "horizon/WaylandWindow.hpp"
#include "horizon/EventsManager.hpp"
#include <string>

namespace horizon::arkfm
{
    struct NewFolderEvent : public EventContext
    {
        std::string folder_name;
    };

    class NewFolderDialog : public WaylandWindow
    {
    public:
        NewFolderDialog();
        ~NewFolderDialog() override = default;

        EventsManager<NewFolderEvent> when_accepted;
        EventsManager<EventContext> when_cancelled;

    private:
        void setup_ui();
    };
} // namespace horizon::arkfm
