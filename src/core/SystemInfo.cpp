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
} // namespace horizon
