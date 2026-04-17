#pragma once
#include "DiskPartition.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace horizon::disks
{
    /**
     * @brief Represents a physical disk device.
     */
    class DiskDevice
    {
    public:
        std::string name;          // e.g., "sda"
        std::string device_path;   // e.g., "/dev/sda"
        std::string vendor;
        std::string model;
        std::string serial;
        
        uint64_t capacity{0};      // in bytes
        bool is_removable{false};
        bool is_ssd{false};
        
        std::string udisks_path;       // Block device path
        std::string udisks_drive_path; // Drive object path
        
        std::vector<std::unique_ptr<DiskPartition>> partitions;

        std::string full_model_name() const;
        std::string human_capacity() const;
    };
} // namespace horizon::disks
