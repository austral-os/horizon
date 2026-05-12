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

    MemoryUsage ProcessManager::get_memory_usage()
    {
        MemoryUsage usage = {0};
        std::ifstream file("/proc/meminfo");
        std::string line;
        
        uint64_t mem_free = 0;
        uint64_t mem_available = 0;
        uint64_t swap_free = 0;

        while (std::getline(file, line))
        {
            std::string key;
            uint64_t value;
            std::string unit;
            std::stringstream ss(line);
            ss >> key >> value >> unit;

            if (key == "MemTotal:") usage.total_physical = value * 1024;
            else if (key == "MemFree:") mem_free = value * 1024;
            else if (key == "MemAvailable:") mem_available = value * 1024;
            else if (key == "Cached:") usage.cached = value * 1024;
            else if (key == "Buffers:") usage.buffers = value * 1024;
            else if (key == "SwapTotal:") usage.total_swap = value * 1024;
            else if (key == "SwapFree:") swap_free = value * 1024;
        }

        usage.free_physical = mem_free;
        usage.available_physical = mem_available;
        usage.used_physical = usage.total_physical - mem_available;
        usage.used_swap = usage.total_swap - swap_free;
        usage.total_virtual = usage.total_physical + usage.total_swap;
        
        if (usage.total_physical > 0) {
            usage.used_percent = (double)usage.used_physical / usage.total_physical * 100.0;
        }

        return usage;
    }

    EnergyUsage ProcessManager::get_energy_usage()
    {
        EnergyUsage usage = {0};
        usage.has_battery = false;
        usage.on_ac = false;

        // Check AC adapter
        std::ifstream ac_file("/sys/class/power_supply/AC/online");
        if (ac_file.is_open()) {
            int online;
            ac_file >> online;
            usage.on_ac = (online == 1);
        } else {
            // Try common alternative names for AC
             std::ifstream ac_file2("/sys/class/power_supply/ACAD/online");
             if (ac_file2.is_open()) {
                 int online;
                 ac_file2 >> online;
                 usage.on_ac = (online == 1);
             }
        }

        // Check Battery (usually BAT0 or BAT1)
        std::string bat_path = "/sys/class/power_supply/BAT0/";
        std::ifstream bat_test(bat_path + "status");
        if (!bat_test.is_open()) {
            bat_path = "/sys/class/power_supply/BAT1/";
            bat_test.open(bat_path + "status");
        }

        if (bat_test.is_open()) {
            usage.has_battery = true;
            std::getline(bat_test, usage.status);
            
            auto read_val = [&](const std::string& node) -> uint64_t {
                std::ifstream f(bat_path + node);
                uint64_t val = 0;
                f >> val;
                return val;
            };

            uint64_t capacity = read_val("capacity");
            usage.percentage = (double)capacity;
            
            usage.cycle_count = (int)read_val("cycle_count");
            
            uint64_t energy_full = read_val("energy_full");
            uint64_t energy_full_design = read_val("energy_full_design");
            if (energy_full_design > 0) {
                usage.health_percent = (double)energy_full / energy_full_design * 100.0;
            }

            uint64_t energy_now = read_val("energy_now");
            uint64_t power_now = read_val("power_now");

            if (power_now > 0) {
                if (usage.status == "Discharging") {
                    usage.time_to_empty_mins = (int)((double)energy_now / power_now * 60.0);
                } else if (usage.status == "Charging") {
                    usage.time_to_full_mins = (int)((double)(energy_full - energy_now) / power_now * 60.0);
                }
            }
        }

        return usage;
    }

    DiskUsage ProcessManager::get_disk_usage()
    {
        DiskUsage usage = {0};
        std::ifstream file("/proc/diskstats");
        std::string line;

        uint64_t current_total_read_sectors = 0;
        uint64_t current_total_write_sectors = 0;
        uint64_t current_total_reads = 0;
        uint64_t current_total_writes = 0;

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            int major, minor;
            std::string name;
            uint64_t reads, reads_merged, read_sectors, read_time;
            uint64_t writes, writes_merged, write_sectors, write_time;

            if (!(ss >> major >> minor >> name >> reads >> reads_merged >> read_sectors >> read_time >> writes >> writes_merged >> write_sectors >> write_time))
                continue;

            // Only count physical disks, not partitions (simplified)
            // Typically sda, sdb, nvme0n1, etc.
            bool is_disk = false;
            if (name.find("sd") == 0 && name.length() == 3) is_disk = true;
            else if (name.find("nvme") == 0 && name.find("n") != std::string::npos && name.find("p") == std::string::npos) is_disk = true;

            if (is_disk) {
                current_total_read_sectors += read_sectors;
                current_total_write_sectors += write_sectors;
                current_total_reads += reads;
                current_total_writes += writes;
            }
        }

        usage.total_read_bytes = current_total_read_sectors * 512;
        usage.total_written_bytes = current_total_write_sectors * 512;
        usage.reads_completed = current_total_reads;
        usage.writes_completed = current_total_writes;

        // Calculate speed
        static auto last_call_time = std::chrono::steady_clock::now();
        static DiskPoint last_total_point = {0, 0, 0, 0};

        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_call_time).count();
        last_call_time = now;

        if (elapsed > 0 && last_total_point.read_sectors > 0) {
            uint64_t delta_read = current_total_read_sectors - last_total_point.read_sectors;
            uint64_t delta_write = current_total_write_sectors - last_total_point.write_sectors;
            
            usage.read_speed_kb = (delta_read * 512.0) / (elapsed * 1024.0);
            usage.write_speed_kb = (delta_write * 512.0) / (elapsed * 1024.0);
        }

        last_total_point = {current_total_read_sectors, current_total_write_sectors, current_total_reads, current_total_writes};

        return usage;
    }
} // namespace horizon
