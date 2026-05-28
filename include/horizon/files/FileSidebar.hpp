#pragma once
#include "horizon/Sidebar.hpp"
#include <horizon/disks/DiskManager.hpp>
#include <horizon/storage/RemoteManagerBase.hpp>
#include <horizon/Menu.hpp>
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

        storage::RemoteManagerBase* remote_storage() { return m_remote_manager; }
        void set_remote_storage(storage::RemoteManagerBase* manager);

        EventsManager<UnmountEventContext> when_resource_unmounted;

    private:
        void setup_monitoring();
        
        disks::DiskManager m_disk_manager;
        storage::RemoteManagerBase* m_remote_manager{nullptr};
        std::unique_ptr<Menu> m_context_menu;
    };
} // namespace horizon::files
