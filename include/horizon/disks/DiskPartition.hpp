#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace horizon::disks
{
    /**
     * @brief Represents a logical partition on a disk.
     */
    struct DiskPartition
    {
        std::string name;          // e.g., "sda1"
        std::string device_path;   // e.g., "/dev/sda1"
        std::string label;
        std::string uuid;
        std::string filesystem;    // e.g., "ext4", "ntfs"
        std::string mount_point;
        std::string udisks_path;   // D-Bus object path
        
        uint64_t capacity{0};      // in bytes
        uint64_t used{0};          // in bytes
        uint64_t offset{0};        // offset in bytes from disk start
        bool is_mounted{false};
        bool is_read_only{false};
        
        // Helper to get human readable size
        std::string human_capacity() const;
        std::string human_used() const;
        std::string human_available() const;
    };
} // namespace horizon::disks
