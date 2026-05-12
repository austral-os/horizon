#pragma once
#include "horizon/Sidebar.hpp"
#include <horizon/disks/DiskManager.hpp>

namespace horizon::files
{
    class FileSidebar : public Sidebar
    {
    public:
        FileSidebar();
        ~FileSidebar() override = default;

    private:
        void refresh_devices();
        void setup_monitoring();
        
        disks::DiskManager m_disk_manager;
    };
} // namespace horizon::files
