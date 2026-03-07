#pragma once

#include <string>

namespace horizon
{
    class SystemInfo
    {
    public:
        static std::string get_model();
        static std::string get_cpu();
        static std::string get_ram();
        static std::string get_graphics();
        static std::string get_os();
        static std::string get_kernel();
    };
} // namespace horizon
