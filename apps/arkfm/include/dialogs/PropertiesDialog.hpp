#pragma once

#include "horizon/WaylandWindow.hpp"
#include "horizon/arkutils/FileInfo.hpp"
#include "horizon/EventsManager.hpp"

namespace horizon::arkfm
{
    class PropertiesDialog : public WaylandWindow
    {
    public:
        PropertiesDialog(const arkutils::FileInfo &file_info);
        ~PropertiesDialog() = default;

        EventsManager<EventContext> when_accepted;
        EventsManager<EventContext> when_cancelled;

    private:
        void setup_ui();
        arkutils::FileInfo m_file_info;
    };
} // namespace horizon::arkfm
