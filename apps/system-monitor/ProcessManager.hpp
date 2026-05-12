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
        std::vector<double> cores;
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
