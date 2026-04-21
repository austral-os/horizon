#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/TreeView.hpp>
#include <string>

namespace horizon::installer
{
    /**
     * @brief Page 3: Disk Selection
     */
    class DiskPage : public Widget
    {
    public:
        DiskPage();
        ~DiskPage() override = default;

        std::string selected_device_str;
        
        EventsManager<EventContext> when_install;
        EventsManager<EventContext> when_back;

    private:
        TreeView* m_disk_tree{nullptr};
        void refresh_disks();
    };
} // namespace horizon::installer
