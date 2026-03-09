#include <horizon/FormatUtils.hpp>
#include <iomanip>
#include <sstream>

namespace horizon
{
    std::string format_bytes(double bytes)
    {
        const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
        int i = 0;
        double display_bytes = bytes;
        while (display_bytes >= 1024 && i < 5)
        {
            display_bytes /= 1024;
            i++;
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << display_bytes << " " << units[i];
        return ss.str();
    }
} // namespace horizon
