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

    struct MemorySlotInfo
    {
        bool occupied;
        uint64_t capacity; // in bytes
        std::string type;  // e.g., DDR3
        uint32_t speed;    // in MHz
    };

    struct MemoryInfo
    {
        uint64_t total_capacity; // in bytes
        uint32_t total_slots;
        std::vector<MemorySlotInfo> slots;
    };

    struct MonitorMode
    {
        int width;
        int height;
        float refresh_rate;
    };

    struct MonitorInfo
    {
        std::string conn_name;
        std::string model;
        int width;
        int height;
        int x;
        int y;
        std::vector<MonitorMode> modes;
        int current_mode_index = -1;
        int rotation = 0; // 0, 90, 180, 270
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
        static MemoryInfo get_memory_info();
        static std::vector<MonitorInfo> get_monitors();
    };
} // namespace horizon
