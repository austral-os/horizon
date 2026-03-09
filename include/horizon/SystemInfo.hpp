#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace horizon
{
    struct PartitionInfo
    {
        std::string name;
        std::string mount_point;
        uint64_t capacity; // in bytes
        uint64_t used;     // in bytes
    };

    struct DiskInfo
    {
        std::string brand;
        std::string model;
        uint64_t capacity; // in bytes
        std::vector<PartitionInfo> partitions;
    };

    class SystemInfo
    {
    public:
        static std::string get_model();
        static std::string get_cpu();
        static std::string get_ram();
        static std::string get_graphics();
        static std::string get_os();
        static std::string get_kernel();
        static std::string get_monitor_name();
        static std::string get_monitor_model();
        static std::string get_monitor_resolution();

        static DiskInfo get_os_disk_info();
    };
} // namespace horizon
