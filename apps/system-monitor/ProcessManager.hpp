#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <map>
#include <chrono>

namespace horizon
{
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
        bool terminate_process(int pid);

    private:
        void* m_info{nullptr}; // Pointer to struct pids_info
        std::map<int, uint64_t> m_prev_tics;
        std::chrono::steady_clock::time_point m_prev_time;
    };
} // namespace horizon
