#pragma once

#include <string>
#include <vector>
#include <cstdint>

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
        // Internal state for libproc2 if needed
    };
} // namespace horizon
