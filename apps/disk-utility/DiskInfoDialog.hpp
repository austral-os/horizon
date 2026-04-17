#pragma once
#include <horizon/WaylandWindow.hpp>
#include <horizon/SystemInfo.hpp>

namespace horizon::disks
{
    /**
     * @brief A dialog window that shows detailed graphical information about a disk.
     */
    class DiskInfoDialog : public WaylandWindow
    {
    public:
        DiskInfoDialog(const DiskInfo& info);
        ~DiskInfoDialog() override = default;
    };
} // namespace horizon::disks
