#include "ProcessManager.hpp"
#include <libproc2/pids.h>
#include <signal.h>
#include <pwd.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <dirent.h>

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

    CPUUsage ProcessManager::get_cpu_usage()
    {
        std::ifstream file("/proc/stat");
        std::string line;
        CPUUsage usage{0.0, 0.0, 0.0, 0.0, 0, 0, {}};
        
        struct CPUDetailed {
            uint64_t user;
            uint64_t system;
            uint64_t idle;
            uint64_t total;
        };
        static CPUDetailed prev_detailed{0, 0, 0, 0};

        std::vector<CPUPoint> current_cores;
        CPUPoint current_total{0, 0};
        CPUDetailed current_detailed{0, 0, 0, 0};

        while (std::getline(file, line))
        {
            if (line.compare(0, 3, "cpu") == 0)
            {
                std::stringstream ss(line);
                std::string cpu_label;
                ss >> cpu_label;

                uint64_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
                ss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;

                uint64_t idle_time = idle + iowait;
                uint64_t user_time = user + nice + guest + guest_nice;
                uint64_t system_time = system + irq + softirq + steal;
                uint64_t total_time = user_time + system_time + idle_time;

                if (cpu_label == "cpu")
                {
                    current_total = {idle_time, total_time};
                    current_detailed = {user_time, system_time, idle_time, total_time};
                }
                else
                {
                    current_cores.push_back({idle_time, total_time});
                }
            }
        }

        // Calculate detailed %
        if (prev_detailed.total != 0)
        {
            uint64_t total_delta = current_detailed.total - prev_detailed.total;
            if (total_delta != 0)
            {
                usage.total = (1.0 - (double)(current_detailed.idle - prev_detailed.idle) / total_delta) * 100.0;
                usage.user = (double)(current_detailed.user - prev_detailed.user) / total_delta * 100.0;
                usage.system = (double)(current_detailed.system - prev_detailed.system) / total_delta * 100.0;
                usage.idle = (double)(current_detailed.idle - prev_detailed.idle) / total_delta * 100.0;
            }
        }
        prev_detailed = current_detailed;
        m_prev_total_cpu = {current_detailed.idle, current_detailed.total};

        // Calculate Cores %
        // ... (rest of core calculation)
        if (m_prev_cores_cpu.size() == current_cores.size())
        {
            for (size_t i = 0; i < current_cores.size(); ++i)
            {
                uint64_t idle_delta = current_cores[i].idle - m_prev_cores_cpu[i].idle;
                uint64_t total_delta = current_cores[i].total - m_prev_cores_cpu[i].total;
                if (total_delta != 0)
                    usage.cores.push_back((1.0 - (double)idle_delta / total_delta) * 100.0);
                else
                    usage.cores.push_back(0.0);
            }
        }
        else
        {
            usage.cores.assign(current_cores.size(), 0.0);
        }
        m_prev_cores_cpu = current_cores;

        // Get process and thread counts
        // A bit slow but accurate: read /proc
        int procs = 0;
        int threads = 0;
        std::ifstream loadavg("/proc/loadavg");
        std::string junk;
        std::string procs_running_total;
        // Format: 0.00 0.00 0.00 1/820 12345
        loadavg >> junk >> junk >> junk >> procs_running_total;
        size_t slash = procs_running_total.find('/');
        if (slash != std::string::npos) {
            usage.total_threads = std::stoi(procs_running_total.substr(slash + 1));
        }
        
        // Count processes by counting directories in /proc
        usage.total_processes = 0;
        if (auto dir = opendir("/proc")) {
            while (auto entry = readdir(dir)) {
                if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                    usage.total_processes++;
                }
            }
            closedir(dir);
        }

        return usage;
    }
}
