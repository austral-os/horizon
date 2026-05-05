#include <chrono>
#include <filesystem>
#include <fstream>
#include <horizon/ConfigManager.hpp>
#include <horizon/Logger.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace horizon;

/**
 * Helper to get the absolute path for a configuration file in ~/.config/horizon/
 */
std::string get_config_path(const std::string &filename)
{
    const char *home = std::getenv("HOME");
    if (!home)
        return filename;

    std::filesystem::path p(home);
    p /= ".config/horizon";
    p /= filename;
    return p.string();
}

/**
 * Check if the system is currently connected to an AC power source.
 */
bool is_on_ac()
{
    try
    {
        if (std::filesystem::exists("/sys/class/power_supply"))
        {
            for (const auto &entry : std::filesystem::directory_iterator("/sys/class/power_supply"))
            {
                std::string name = entry.path().filename().string();
                if (name.find("AC") == 0 || name.find("ADP") == 0)
                {
                    std::ifstream f(entry.path() / "online");
                    int online = 0;
                    if (f >> online)
                        return online == 1;
                }
            }
        }
    }
    catch (...)
    {
    }
    return true; // Default to AC
}

/**
 * Apply screen brightness using brightnessctl.
 */
void apply_brightness(int value)
{
    std::string cmd = "brightnessctl set " + std::to_string(value) + "%";
    int res = system(cmd.c_str());
    (void)res;
}

/**
 * Apply battery charge thresholds to sysfs nodes.
 */
void apply_battery_limits(int start, int end)
{
    try
    {
        for (const auto &entry : std::filesystem::directory_iterator("/sys/class/power_supply"))
        {
            std::string name = entry.path().filename().string();
            if (name.find("BAT") == 0)
            {
                std::string bat_path = entry.path().string();
                std::vector<std::pair<std::string, std::string>> pairs = {
                    {"charge_control_start_threshold", "charge_control_end_threshold"},
                    {"charge_start_threshold", "charge_stop_threshold"}};

                for (auto &p : pairs)
                {
                    std::string f1 = bat_path + "/" + p.first;
                    std::string f2 = bat_path + "/" + p.second;
                    if (std::filesystem::exists(f1))
                    {
                        std::ofstream s1(f1);
                        s1 << start;
                        std::ofstream s2(f2);
                        s2 << end;
                    }
                }
            }
        }
    }
    catch (...)
    {
    }
}

int main(int argc, char **argv)
{
    // Initialize Horizon Logger
    Logger::instance().init("horizon-powerd");
    LOG_INFO << "Horizon Power Daemon started";

    bool last_ac_state = !is_on_ac(); // Force first check
    std::filesystem::file_time_type last_config_time;
    std::string config_file = get_config_path("power.json");

    while (true)
    {
        bool current_ac = is_on_ac();
        bool config_changed = false;

        // Check if config file was modified
        if (std::filesystem::exists(config_file))
        {
            auto current_time = std::filesystem::last_write_time(config_file);
            if (current_time != last_config_time)
            {
                config_changed = true;
                last_config_time = current_time;
            }
        }

        // Apply settings if power state changed or config changed
        if (current_ac != last_ac_state || config_changed)
        {
            ConfigManager config(config_file);
            if (config.load())
            {
                auto power = config.get_section("power");

                // 1. Handle Brightness
                std::string mode = current_ac ? "ac" : "battery";
                LOG_INFO << "Applying brightness for mode: " << mode;
                if (power.contains(mode))
                {
                    auto section = power[mode];
                    if (section.contains("brightness"))
                    {
                        int brightness = section["brightness"].get<int>();
                        LOG_INFO << "Applying brightness: " << brightness << "%";
                        apply_brightness(brightness);
                    }
                }

                // 2. Handle Battery Limits (Always apply to be sure)
                if (power.contains("battery"))
                {
                    auto b = power["battery"];
                    if (b.contains("charge_limit_min") && b.contains("charge_limit_max"))
                    {
                        LOG_INFO << "Applying battery limits: " << b["charge_limit_min"].get<int>()
                                 << "% - " << b["charge_limit_max"].get<int>() << "%";
                        apply_battery_limits(b["charge_limit_min"].get<int>(),
                                             b["charge_limit_max"].get<int>());
                    }
                }
            }
            last_ac_state = current_ac;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
