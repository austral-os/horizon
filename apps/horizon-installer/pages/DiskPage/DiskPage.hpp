#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/IconView.hpp>
#include <string>

namespace horizon::installer
{
    struct DiskData {
        std::string name;
        std::string path;
        std::string capacity;
        bool is_ssd;
    };

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
        IconView<DiskData>* m_disk_tree{nullptr};
        void refresh_disks();
    };
} // namespace horizon::installer
