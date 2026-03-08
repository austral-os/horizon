#include "horizon/SystemInfo.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
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
} // namespace horizon
