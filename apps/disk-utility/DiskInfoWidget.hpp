#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon-disk-utilities/DiskPartition.hpp>
#include <horizon-disk-utilities/DiskDevice.hpp>

namespace horizon::disks
{
    class DiskInfoWidget : public Widget
    {
    public:
        DiskInfoWidget();
        ~DiskInfoWidget() = default;

        void update_info(const DiskPartition& partition);
        void update_info(const DiskDevice& device);

    private:
        Label* m_mount_point_lbl{nullptr};
        Label* m_format_lbl{nullptr};
        Label* m_capacity_lbl{nullptr};
        Label* m_available_lbl{nullptr};
        Label* m_used_lbl{nullptr};
    };
} // namespace horizon::disks
