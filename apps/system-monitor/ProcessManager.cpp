#include "ProcessManager.hpp"
#include <libproc2/pids.h>
#include <signal.h>
#include <pwd.h>
#include <unistd.h>
#include <iostream>

namespace horizon
{
    ProcessManager::ProcessManager() {}
    ProcessManager::~ProcessManager() {}

    std::vector<ProcessInfo> ProcessManager::get_processes()
    {
        std::vector<ProcessInfo> processes;
        
        struct pids_info *info = NULL;
        enum pids_item items[] = { 
            PIDS_ID_PID, 
            PIDS_CMD, 
            PIDS_MEM_RES, 
            PIDS_UTILIZATION, 
            PIDS_ID_EUSER,
            PIDS_CMDLINE
        };
        
        if (procps_pids_new(&info, items, 6) < 0) {
            std::cerr << "Failed to open libproc2" << std::endl;
            return processes;
        }

        // Use reap to get all processes at once
        struct pids_fetch *fetch = procps_pids_reap(info, PIDS_FETCH_TASKS_ONLY);
        if (fetch && fetch->stacks) {
            for (int i = 0; fetch->stacks[i]; ++i) {
                struct pids_stack *stack = fetch->stacks[i];
                ProcessInfo p;
                
                p.pid = PIDS_VAL(0, s_int, stack, info);
                
                const char* name_ptr = PIDS_VAL(1, str, stack, info);
                p.name = name_ptr ? name_ptr : "";
                
                p.memory_bytes = (uint64_t)PIDS_VAL(2, ul_int, stack, info) * 1024;
                
                p.cpu_percent = PIDS_VAL(3, real, stack, info);
                
                const char* user_ptr = PIDS_VAL(4, str, stack, info);
                p.user = user_ptr ? user_ptr : "";
                
                const char* cmd_ptr = PIDS_VAL(5, str, stack, info);
                p.command = cmd_ptr ? cmd_ptr : p.name;

                processes.push_back(p);
            }
        }

        procps_pids_unref(&info);
        return processes;
    }

    bool ProcessManager::terminate_process(int pid)
    {
        return kill(pid, SIGTERM) == 0;
    }
}
