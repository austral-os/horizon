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
} // namespace horizon
