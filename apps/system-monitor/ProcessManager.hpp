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
    };
} // namespace horizon
