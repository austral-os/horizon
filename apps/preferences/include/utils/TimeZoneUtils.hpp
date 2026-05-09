#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <memory>

namespace horizon::preferences
{
    struct TimeZone
    {
        std::string id;
        std::string name;
    };

    struct TimeZoneSelection
    {
        std::string id;
        std::string name;
        bool is_default{false};
    };

    class TimeZoneUtils
    {
    public:
        static std::vector<TimeZone> get_all_timezones()
        {
            std::vector<TimeZone> timezones;
            FILE* pipe = popen("timedatectl list-timezones", "r");
            if (!pipe) return timezones;

            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                std::string tz(buffer);
                if (!tz.empty() && tz.back() == '\n') tz.pop_back();
                if (!tz.empty()) {
                    timezones.push_back({tz, tz});
                }
            }
            pclose(pipe);
            return timezones;
        }
    };
} // namespace horizon::preferences
