#include "ProcessManager.hpp"
#include <libproc2/pids.h>
#include <signal.h>
#include <pwd.h>
#include <unistd.h>
#include <iostream>

namespace horizon
{
    ProcessManager::ProcessManager() {}
    ProcessManager::~ProcessManager()
    {
        if (m_info)
        {
            struct pids_info *info = static_cast<struct pids_info *>(m_info);
            procps_pids_unref(&info);
        }
    }

    std::vector<ProcessInfo> ProcessManager::get_processes()
    {
        auto now = std::chrono::steady_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - m_prev_time).count();
        m_prev_time = now;
        
        // Ensure we don't divide by zero on first run
        if (elapsed_sec <= 0) elapsed_sec = 1.0;

        std::vector<ProcessInfo> processes;
        
        struct pids_info *info = static_cast<struct pids_info *>(m_info);
        enum pids_item items[] = { 
            PIDS_ID_PID, 
            PIDS_CMD, 
            PIDS_SMAP_PSS, 
            PIDS_TICS_ALL,
            PIDS_ID_EUSER,
            PIDS_CMDLINE,
            PIDS_MEM_RES
        };
        
        if (!info) {
            if (procps_pids_new(&info, items, 7) < 0) {
                std::cerr << "Failed to open libproc2" << std::endl;
                return processes;
            }
            m_info = info;
        }

        // Use reap to get all processes (tasks in libproc2 terms)
        struct pids_fetch *fetch = procps_pids_reap(info, PIDS_FETCH_TASKS_ONLY);
        
        std::map<int, uint64_t> current_tics_map;
        static int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
        static long hertz = sysconf(_SC_CLK_TCK);

        if (fetch && fetch->stacks) {
            for (int i = 0; fetch->stacks[i]; ++i) {
                struct pids_stack *stack = fetch->stacks[i];
                ProcessInfo p;
                
                p.pid = PIDS_VAL(0, s_int, stack, info);
                
                const char* name_ptr = PIDS_VAL(1, str, stack, info);
                p.name = name_ptr ? name_ptr : "";
                
                // Use PSS if available, fallback to RSS
                uint64_t pss_kib = PIDS_VAL(2, ul_int, stack, info);
                if (pss_kib == 0) {
                    pss_kib = PIDS_VAL(6, ul_int, stack, info);
                }
                p.memory_bytes = pss_kib * 1024;
                
                // Calculate instantaneous CPU percentage
                uint64_t current_tics = PIDS_VAL(3, ull_int, stack, info);
                current_tics_map[p.pid] = current_tics;

                if (m_prev_tics.count(p.pid)) {
                    uint64_t delta_tics = current_tics - m_prev_tics[p.pid];
                    double cpu_time_sec = (double)delta_tics / hertz;
                    p.cpu_percent = (cpu_time_sec / elapsed_sec) * 100.0 / num_cores;
                } else {
                    p.cpu_percent = 0.0;
                }
                
                const char* user_ptr = PIDS_VAL(4, str, stack, info);
                p.user = user_ptr ? user_ptr : "";
                
                const char* cmd_ptr = PIDS_VAL(5, str, stack, info);
                p.command = cmd_ptr ? cmd_ptr : p.name;

                processes.push_back(p);
            }
        }

        m_prev_tics = std::move(current_tics_map);
        return processes;
    }

    bool ProcessManager::terminate_process(int pid)
    {
        return kill(pid, SIGTERM) == 0;
    }
}
