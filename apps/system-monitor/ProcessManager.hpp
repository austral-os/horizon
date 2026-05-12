#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <map>
#include <chrono>

namespace horizon
{
    struct CPUUsage
    {
        double total;
        double user;
        double system;
        double idle;
        int total_threads;
        int total_processes;
        std::vector<double> cores;
    };

    struct MemoryUsage
    {
        uint64_t total_physical;
        uint64_t used_physical;
        uint64_t free_physical;
        uint64_t available_physical;
        uint64_t cached;
        uint64_t buffers;
        uint64_t total_swap;
        uint64_t used_swap;
        uint64_t total_virtual;
        double used_percent;
    };

    struct EnergyUsage
    {
        bool has_battery;
        double percentage;
        std::string status;
        int cycle_count;
        double health_percent;
        int time_to_empty_mins;
        int time_to_full_mins;
        bool on_ac;
    };

    struct DiskUsage
    {
        uint64_t reads_completed;
        uint64_t writes_completed;
        double read_speed_kb;
        double write_speed_kb;
        uint64_t total_read_bytes;
        uint64_t total_written_bytes;
    };

    struct NetworkUsage
    {
        uint64_t rx_packets;
        uint64_t tx_packets;
        double rx_packets_per_sec;
        double tx_packets_per_sec;
        uint64_t rx_bytes;
        uint64_t tx_bytes;
        double rx_kb_per_sec;
        double tx_kb_per_sec;
    };

    struct ProcessInfo
    {
        int pid;
        std::string name;
        uint64_t memory_bytes;
        double cpu_percent;
        std::string user;
        std::string command;
    };

    class ProcessManager
    {
    public:
        ProcessManager();
        ~ProcessManager();

        std::vector<ProcessInfo> get_processes();
        CPUUsage get_cpu_usage();
        MemoryUsage get_memory_usage();
        EnergyUsage get_energy_usage();
        DiskUsage get_disk_usage();
        NetworkUsage get_network_usage();
        bool terminate_process(int pid);

    private:
        void* m_info{nullptr}; // Pointer to struct pids_info
        std::map<int, uint64_t> m_prev_tics;
        std::chrono::steady_clock::time_point m_prev_time;
        
        struct CPUPoint {
            uint64_t idle;
            uint64_t total;
        };
        CPUPoint m_prev_total_cpu{0, 0};
        std::vector<CPUPoint> m_prev_cores_cpu;

        struct DiskPoint {
            uint64_t read_sectors;
            uint64_t write_sectors;
            uint64_t reads_completed;
            uint64_t writes_completed;
        };
        std::map<std::string, DiskPoint> m_prev_disk_stats;

        struct NetPoint {
            uint64_t rx_bytes;
            uint64_t tx_bytes;
            uint64_t rx_packets;
            uint64_t tx_packets;
        };
        NetPoint m_prev_net_stats{0, 0, 0, 0};
    };
} // namespace horizon
