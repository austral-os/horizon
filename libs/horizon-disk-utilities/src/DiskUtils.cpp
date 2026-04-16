#include "horizon-disk-utilities/DiskDevice.hpp"
#include "horizon-disk-utilities/DiskPartition.hpp"
#include <iomanip>
#include <sstream>

namespace horizon::disks
{
    static std::string to_human_size(uint64_t bytes)
    {
        const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
        int unit_idx = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024 && unit_idx < 5)
        {
            size /= 1024;
            unit_idx++;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << size << " " << units[unit_idx];
        return ss.str();
    }

    std::string DiskPartition::human_capacity() const { return to_human_size(capacity); }
    std::string DiskPartition::human_used() const { return to_human_size(used); }

    std::string DiskDevice::full_model_name() const
    {
        if (vendor == "Generic" || vendor.empty()) return model;
        return vendor + " " + model;
    }

    std::string DiskDevice::human_capacity() const { return to_human_size(capacity); }

} // namespace horizon::disks
