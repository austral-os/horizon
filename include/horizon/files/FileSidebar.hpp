#pragma once
#include "horizon/Sidebar.hpp"
#include <horizon/disks/DiskManager.hpp>
#include <horizon/storage/RemoteManager.hpp>
#include <memory>

namespace horizon::files
{
    struct UnmountEventContext : public EventContext
    {
        std::string mount_path;
    };

    class FileSidebar : public Sidebar
    {
    public:
        FileSidebar();
        ~FileSidebar() override = default;

        void refresh_devices();

        storage::RemoteManager* remote_storage() { return m_remote_manager.get(); }

        EventsManager<UnmountEventContext> when_resource_unmounted;

    private:
        void setup_monitoring();
        
        disks::DiskManager m_disk_manager;
        std::unique_ptr<storage::RemoteManager> m_remote_manager;
    };
} // namespace horizon::files
