#include "horizon/SystemInfo.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/statvfs.h>
#include <sys/utsname.h>

namespace horizon
{
    static std::string read_file(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());
        return content;
    }

    std::string SystemInfo::get_model()
    {
        std::string model = read_file("/sys/devices/virtual/dmi/id/product_name");
        if (model.empty())
            model = "Generic Device";
        return model;
    }

    std::string SystemInfo::get_cpu()
    {
        std::ifstream file("/proc/cpuinfo");
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("model name") != std::string::npos)
            {
                size_t pos = line.find(':');
                if (pos != std::string::npos)
                {
                    return line.substr(pos + 2);
                }
            }
        }
        return "Unknown CPU";
    }

    std::string SystemInfo::get_ram()
    {
        std::ifstream file("/proc/meminfo");
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("MemTotal") != std::string::npos)
            {
                size_t pos = line.find(':');
                if (pos != std::string::npos)
                {
                    std::string total = line.substr(pos + 1);
                    // Trim leading whitespace
                    total.erase(0, total.find_first_not_of(" \t"));
                    return total;
                }
            }
        }
        return "Unknown RAM";
    }

    std::string SystemInfo::get_graphics()
    {
        // Simple heuristic for primary graphics card
        std::ifstream file("/sys/class/drm/card0/device/uevent");
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("PCI_ID") != std::string::npos)
            {
                // This is a placeholder, getting the actual name requires a PCI ID database or
                // lspci
                return "PCI GPU (" + line + ")";
            }
        }
        return "Standard Graphics";
    }

    std::string SystemInfo::get_os()
    {
        std::ifstream file("/etc/os-release");
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("PRETTY_NAME=") == 0)
            {
                size_t start = line.find('"');
                size_t end = line.rfind('"');
                if (start != std::string::npos && end != std::string::npos && start != end)
                {
                    return line.substr(start + 1, end - start - 1);
                }
            }
        }
        return "Linux";
    }

    std::string SystemInfo::get_kernel()
    {
        struct utsname buffer;
        if (uname(&buffer) != 0)
        {
            return "Unknown Kernel";
        }
        return std::string(buffer.release);
    }

    std::string SystemInfo::get_monitor_name()
    {
        for (int i = 0; i < 10; ++i)
        {
            std::string path = "/sys/class/drm/card0-";
            // Check common connector types
            const char *connectors[] = {"eDP-1", "HDMI-A-1", "DP-1", "HDMI-A-2", "DP-2"};
            for (const char *conn : connectors)
            {
                std::string status = read_file(path + conn + "/status");
                if (status == "connected")
                {
                    return conn;
                }
            }
        }
        return "Unknown Monitor";
    }

    std::string SystemInfo::get_monitor_model()
    {
        std::string conn = get_monitor_name();
        if (conn == "Unknown Monitor")
            return "Generic Monitor";

        std::string edid_path = "/sys/class/drm/card0-" + conn + "/edid";
        std::ifstream file(edid_path, std::ios::binary);
        if (!file.is_open())
            return "Generic Monitor";

        char buffer[128];
        if (!file.read(buffer, 128))
            return "Generic Monitor";

        // EDID extraction of monitor name (Descriptor blocks at 0x36, 0x48, 0x5A, 0x6C)
        // This is a simplified extraction looking for the ASCII descriptor (0xFC)
        for (int i = 0; i < 4; ++i)
        {
            int offset = 0x36 + i * 18;
            if (buffer[offset] == 0 && buffer[offset + 1] == 0 && buffer[offset + 2] == 0 &&
                (unsigned char)buffer[offset + 3] == 0xFC)
            {
                std::string name(buffer + offset + 5, 13);
                // Trim trailing spaces and newlines
                name.erase(name.find_last_not_of(" \n\r\t") + 1);
                return name;
            }
        }

        return "Generic Monitor";
    }

    std::string SystemInfo::get_monitor_resolution()
    {
        std::string conn = get_monitor_name();
        if (conn == "Unknown Monitor")
            return "Unknown Resolution";

        std::ifstream file("/sys/class/drm/card0-" + conn + "/modes");
        std::string line;
        if (std::getline(file, line))
        {
            return line;
        }
        return "Unknown Resolution";
    }

    DiskInfo SystemInfo::get_os_disk_info()
    {
        DiskInfo info;
        info.brand = "Unknown";
        info.model = "Unknown Disk";
        info.capacity = 0;

        // 1. Find root dev from /proc/mounts
        std::string root_dev = "";
        std::ifstream mounts("/proc/mounts");
        std::string line;
        while (std::getline(mounts, line))
        {
            if (line.find(" / ") != std::string::npos)
            {
                std::stringstream ss(line);
                ss >> root_dev;
                break;
            }
        }

        if (root_dev.empty())
            return info;

        // 2. Extract disk name
        // /dev/nvme0n1p2 -> nvme0n1
        // /dev/sda1 -> sda
        std::string dev_name = root_dev;
        if (dev_name.find("/dev/") == 0)
            dev_name = dev_name.substr(5);

        std::string disk_name = dev_name;
        // Search for the disk in /sys/block
        namespace fs = std::filesystem;
        bool found_disk = false;

        // Try to find the parent disk by checking /sys/block
        for (const auto &entry : fs::directory_iterator("/sys/block"))
        {
            std::string bname = entry.path().filename().string();
            if (dev_name.find(bname) == 0)
            {
                disk_name = bname;
                found_disk = true;
                break;
            }
        }

        if (!found_disk)
            return info;

        // 3. Get disk info from /sys/block/disk_name
        std::string sys_path = "/sys/block/" + disk_name;

        info.model = read_file(sys_path + "/device/model");
        if (info.model.empty())
            info.model = disk_name;

        info.brand = read_file(sys_path + "/device/vendor");
        if (info.brand.empty())
            info.brand = "Generic";

        std::string size_str = read_file(sys_path + "/size");
        if (!size_str.empty())
        {
            try
            {
                info.capacity = std::stoull(size_str) * 512;
            }
            catch (...)
            {
            }
        }

        // 4. Get partitions from /proc/mounts that belong to this disk
        mounts.clear();
        mounts.seekg(0);
        while (std::getline(mounts, line))
        {
            std::stringstream ss(line);
            std::string path, mount, type;
            if (!(ss >> path >> mount >> type))
                continue;

            if (path.find("/dev/" + disk_name) == 0)
            {
                PartitionInfo p;
                p.name = path;
                p.mount_point = mount;

                struct statvfs vfs;
                if (statvfs(mount.c_str(), &vfs) == 0)
                {
                    p.capacity = (uint64_t)vfs.f_blocks * vfs.f_frsize;
                    p.used = p.capacity - ((uint64_t)vfs.f_bfree * vfs.f_frsize);
                    info.partitions.push_back(p);
                }
            }
        }

        return info;
    }

    MemoryInfo SystemInfo::get_memory_info()
    {
        MemoryInfo info;
        info.total_capacity = 0;
        info.total_slots = 0;

        // 1. Get total RAM from /proc/meminfo
        std::ifstream file("/proc/meminfo");
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("MemTotal") != std::string::npos)
            {
                size_t pos = line.find(':');
                if (pos != std::string::npos)
                {
                    std::stringstream ss(line.substr(pos + 1));
                    uint64_t kb;
                    if (ss >> kb)
                    {
                        info.total_capacity = kb * 1024;
                    }
                }
                break;
            }
        }

        // 2. Try to read from /usr/share/horizon/memory
        std::string config_path = "/usr/share/horizon/memory";
        std::ifstream dmi_file(config_path);

        if (dmi_file.is_open())
        {
            std::string dmi_line;
            MemorySlotInfo current_slot;
            bool in_device = false;
            bool occupied = false;

            while (std::getline(dmi_file, dmi_line))
            {
                // Number Of Devices (DMI type 16)
                if (dmi_line.find("Number Of Devices:") != std::string::npos)
                {
                    size_t pos = dmi_line.find(':');
                    try
                    {
                        info.total_slots = std::stoi(dmi_line.substr(pos + 1));
                    }
                    catch (...)
                    {
                    }
                }

                // Memory Device entry (DMI type 17)
                if (dmi_line.find("Memory Device") != std::string::npos &&
                    dmi_line.find("Physical") == std::string::npos)
                {
                    if (in_device)
                    {
                        current_slot.occupied = occupied;
                        info.slots.push_back(current_slot);
                    }
                    in_device = true;
                    occupied = false;
                    current_slot = {false, 0, "Unknown", 0};
                }

                if (in_device)
                {
                    if (dmi_line.find("Size:") != std::string::npos)
                    {
                        size_t pos = dmi_line.find(':');
                        std::string size_str = dmi_line.substr(pos + 1);
                        if (size_str.find("No Module Installed") == std::string::npos)
                        {
                            occupied = true;
                            std::stringstream ss(size_str);
                            uint64_t val;
                            std::string unit;
                            if (ss >> val >> unit)
                            {
                                if (unit == "GB")
                                    current_slot.capacity = val * 1024LL * 1024LL * 1024LL;
                                else if (unit == "MB")
                                    current_slot.capacity = val * 1024LL * 1024LL;
                            }
                        }
                    }
                    else if (dmi_line.find("Type:") != std::string::npos &&
                             dmi_line.find("Error Correction Type") == std::string::npos &&
                             dmi_line.find("Memory Technology") == std::string::npos)
                    {
                        size_t pos = dmi_line.find(':');
                        current_slot.type = dmi_line.substr(pos + 2);
                        // Trim trailing spaces
                        current_slot.type.erase(current_slot.type.find_last_not_of(" \t\n\r") + 1);
                    }
                    else if (dmi_line.find("Speed:") != std::string::npos &&
                             dmi_line.find("Configured") == std::string::npos &&
                             dmi_line.find("Minimum") == std::string::npos &&
                             dmi_line.find("Maximum") == std::string::npos)
                    {
                        size_t pos = dmi_line.find(':');
                        std::stringstream ss(dmi_line.substr(pos + 1));
                        uint32_t s;
                        if (ss >> s)
                            current_slot.speed = s;
                    }
                }
            }
            // Add last device
            if (in_device)
            {
                current_slot.occupied = occupied;
                info.slots.push_back(current_slot);
            }

            if (!info.slots.empty())
            {
                // Ensure total_slots matches if not found earlier
                if (info.total_slots == 0)
                    info.total_slots = (uint32_t)info.slots.size();
                return info;
            }
        }

        // 3. Fallback to Heuristic (Smart Sum)
        uint64_t total_gb =
            (info.total_capacity + (512 * 1024 * 1024)) / (1024LL * 1024LL * 1024LL);

        auto create_stick = [](uint64_t gb)
        { return MemorySlotInfo{true, gb * 1024LL * 1024LL * 1024LL, "DDR4", 3200}; };

        if (total_gb <= 2)
        {
            info.total_slots = 1;
            info.slots = {create_stick(total_gb)};
        }
        else if (total_gb == 6)
        { // 2 + 4
            info.total_slots = 2;
            info.slots = {create_stick(2), create_stick(4)};
        }
        else if (total_gb == 10)
        { // 2 + 8
            info.total_slots = 2;
            info.slots = {create_stick(2), create_stick(8)};
        }
        else if (total_gb == 12)
        { // 4 + 8
            info.total_slots = 2;
            info.slots = {create_stick(4), create_stick(8)};
        }
        else if (total_gb == 20)
        { // 4 + 16
            info.total_slots = 2;
            info.slots = {create_stick(4), create_stick(16)};
        }
        else if (total_gb == 24)
        { // 8 + 16
            info.total_slots = 2;
            info.slots = {create_stick(8), create_stick(16)};
        }
        else if (total_gb == 36 || total_gb == 40 || total_gb == 44)
        { // 4+32, 8+32, 12+32? Let's be smart.
            info.total_slots = 2;
            uint64_t s1 = 8;
            if (total_gb == 36)
                s1 = 4;
            if (total_gb == 44)
                s1 = 12; // Unusual but can happen
            info.slots = {create_stick(s1), create_stick(total_gb - s1)};
        }
        else if (total_gb == 48)
        { // 16 + 32
            info.total_slots = 2;
            info.slots = {create_stick(16), create_stick(32)};
        }
        else if (total_gb == 72)
        { // 8 + 64
            info.total_slots = 4;
            info.slots = {create_stick(8), create_stick(64), MemorySlotInfo{false, 0, "", 0},
                          MemorySlotInfo{false, 0, "", 0}};
        }
        else if (total_gb == 80)
        { // 16 + 64
            info.total_slots = 2;
            info.slots = {create_stick(16), create_stick(64)};
        }
        else
        {
            // Power of 2 or unknown: Assume symmetric or single stick
            if ((total_gb & (total_gb - 1)) == 0)
            {
                info.total_slots = 2;
                info.slots = {create_stick(total_gb / 2), create_stick(total_gb / 2)};
            }
            else
            {
                info.total_slots = 2;
                info.slots = {create_stick(total_gb / 2), create_stick(total_gb - (total_gb / 2))};
            }
        }

        return info;
    }
    std::vector<MonitorInfo> SystemInfo::get_monitors()
    {
        std::vector<MonitorInfo> monitors;
        namespace fs = std::filesystem;

        for (const auto &entry : fs::directory_iterator("/sys/class/drm"))
        {
            std::string name = entry.path().filename().string();
            if (name.find("card0-") != 0)
                continue;

            std::string status = read_file(entry.path().string() + "/status");
            if (status != "connected")
                continue;

            MonitorInfo info;
            info.conn_name = name.substr(6); // Remove "card0-"
            
            // Get Resolution from modes
            std::ifstream modes_file(entry.path().string() + "/modes");
            std::string mode_line;
            if (std::getline(modes_file, mode_line))
            {
                size_t x_pos = mode_line.find('x');
                if (x_pos != std::string::npos)
                {
                    try {
                        info.width = std::stoi(mode_line.substr(0, x_pos));
                        info.height = std::stoi(mode_line.substr(x_pos + 1));
                    } catch (...) {
                        info.width = 1920;
                        info.height = 1080;
                    }
                }
            }

            // Get Model from EDID
            std::string edid_path = entry.path().string() + "/edid";
            std::ifstream edid_file(edid_path, std::ios::binary);
            info.model = "Generic Monitor";
            if (edid_file.is_open())
            {
                char buffer[128];
                if (edid_file.read(buffer, 128))
                {
                    for (int i = 0; i < 4; ++i)
                    {
                        int offset = 0x36 + i * 18;
                        if (buffer[offset] == 0 && buffer[offset + 1] == 0 && buffer[offset + 2] == 0 &&
                            (unsigned char)buffer[offset + 3] == 0xFC)
                        {
                            std::string mname(buffer + offset + 5, 13);
                            mname.erase(mname.find_last_not_of(" \n\r\t") + 1);
                            info.model = mname;
                            break;
                        }
                    }
                }
            }

            // Get all modes
            std::ifstream all_modes_file(entry.path().string() + "/modes");
            std::string line;
            while (std::getline(all_modes_file, line))
            {
                size_t x_pos = line.find('x');
                if (x_pos != std::string::npos)
                {
                    try {
                        MonitorMode m;
                        m.width = std::stoi(line.substr(0, x_pos));
                        m.height = std::stoi(line.substr(x_pos + 1));
                        m.refresh_rate = 60.0f; // Default for now
                        info.modes.push_back(m);
                        
                        // Check if this is current
                        if (m.width == info.width && m.height == info.height && info.current_mode_index == -1) {
                            info.current_mode_index = (int)info.modes.size() - 1;
                        }
                    } catch (...) {}
                }
            }

            // Placeholder for logical coordinates (0,0 for the first one)
            info.x = monitors.empty() ? 0 : monitors.back().x + monitors.back().width;
            info.y = 0;

            monitors.push_back(info);
        }

        return monitors;
    }
} // namespace horizon
