#include <chrono>
#include <filesystem>
#include <fstream>
#include <horizon/ConfigManager.hpp>
#include <horizon/IdleManager.hpp>
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
 * Get current screen brightness percentage.
 */
int get_brightness_percent()
{
    FILE *pipe = popen("brightnessctl -m | cut -d, -f4 | tr -d '%'", "r");
    if (!pipe)
        return 50;
    char buffer[128];
    int val = 50;
    if (fgets(buffer, sizeof(buffer), pipe))
    {
        val = std::atoi(buffer);
    }
    pclose(pipe);
    return val;
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

uint32_t parse_duration(const std::string &s)
{
    if (s == "never")
        return 0;
    try
    {
        if (s.find("m") != std::string::npos)
        {
            return std::stoul(s) * 60 * 1000;
        }
        if (s.find("s") != std::string::npos)
        {
            return std::stoul(s) * 1000;
        }
    }
    catch (...)
    {
    }
    return 0;
}

class PowerDaemon
{
public:
    PowerDaemon()
    {
        m_config_file = get_config_path("power.json");
    }

    void run()
    {
        LOG_INFO << "Horizon Power Daemon started";
        m_idle_manager.init();

        bool last_ac_state = !is_on_ac(); // Force first check
        std::filesystem::file_time_type last_config_time;

        while (true)
        {
            bool current_ac = is_on_ac();
            bool config_changed = false;

            if (std::filesystem::exists(m_config_file))
            {
                auto current_time = std::filesystem::last_write_time(m_config_file);
                if (current_time != last_config_time)
                {
                    config_changed = true;
                    last_config_time = current_time;
                }
            }

            if (current_ac != last_ac_state || config_changed)
            {
                apply_settings(current_ac);
                last_ac_state = current_ac;
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

private:
    void apply_settings(bool current_ac)
    {
        ConfigManager config(m_config_file);
        if (!config.load())
            return;

        auto power = config.get_section("power");
        std::string mode = current_ac ? "ac" : "battery";

        if (power.contains(mode))
        {
            auto section = power[mode];

            // 1. Brightness
            if (section.contains("brightness") && !m_is_dimmed)
            {
                int brightness = section["brightness"].get<int>();
                LOG_INFO << "Applying brightness: " << brightness << "%";
                apply_brightness(brightness);
            }

            // 2. Idle / Dimming
            if (section.contains("dim_after"))
            {
                std::string dim_after = section["dim_after"].get<std::string>();
                update_idle_settings(dim_after);
            }

            // 3. Turn Off / Lock
            if (section.contains("turn_off_after"))
            {
                std::string turn_off_after = section["turn_off_after"].get<std::string>();
                update_lock_settings(turn_off_after);
            }
        }

        // 4. Battery Limits
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

    void update_idle_settings(const std::string &dim_after)
    {
        uint32_t ms = parse_duration(dim_after);
        if (ms == m_current_dim_timeout)
            return;

        if (m_dim_handle)
        {
            m_idle_manager.remove_idle_timeout(m_dim_handle);
            m_dim_handle = nullptr;
        }

        m_current_dim_timeout = ms;
        if (ms > 0)
        {
            LOG_INFO << "Setting idle dim timeout to " << ms << "ms";
            m_dim_handle = m_idle_manager.add_idle_timeout(ms, [this](bool idle)
                                                           { this->handle_dim_event(idle); });
        }
    }

    void update_lock_settings(const std::string &turn_off_after)
    {
        uint32_t ms = parse_duration(turn_off_after);
        if (ms == m_current_lock_timeout)
            return;

        if (m_lock_handle)
        {
            m_idle_manager.remove_idle_timeout(m_lock_handle);
            m_lock_handle = nullptr;
        }

        m_current_lock_timeout = ms;
        if (ms > 0)
        {
            LOG_INFO << "Setting idle lock timeout to " << ms << "ms";
            m_lock_handle = m_idle_manager.add_idle_timeout(ms, [this](bool idle)
                                                            { this->handle_lock_event(idle); });
        }
    }

    void handle_dim_event(bool idle)
    {
        if (idle)
        {
            if (m_is_dimmed)
                return;
            m_pre_dim_brightness = get_brightness_percent();
            int dim_val = m_pre_dim_brightness / 2;
            if (dim_val < 5)
                dim_val = 5;

            LOG_INFO << "System idle, dimming screen from " << m_pre_dim_brightness << "% to " << dim_val << "%";
            apply_brightness(dim_val);
            m_is_dimmed = true;
        }
        else
        {
            if (!m_is_dimmed)
                return;
            LOG_INFO << "System active, restoring brightness to " << m_pre_dim_brightness << "%";
            apply_brightness(m_pre_dim_brightness);
            m_is_dimmed = false;
        }
    }

    void handle_lock_event(bool idle)
    {
        if (idle)
        {
            LOG_INFO << "System idle (long time), locking session and turning off screen";
            // Lock session
            int res = system("horizon-lock &");
            (void)res;
            
            // Turn off screen (using brightness 0 for now as simple DPMS alternative)
            apply_brightness(0);
        }
        else
        {
            LOG_INFO << "System active, turning screen back on";
            // Restore brightness (dimming handle will handle the exact level)
            if (!m_is_dimmed) {
                apply_brightness(m_pre_dim_brightness);
            }
        }
    }

    std::string m_config_file;
    IdleManager m_idle_manager;
    void *m_dim_handle{nullptr};
    void *m_lock_handle{nullptr};
    uint32_t m_current_dim_timeout{0};
    uint32_t m_current_lock_timeout{0};
    bool m_is_dimmed{false};
    int m_pre_dim_brightness{50};
};

int main(int argc, char **argv)
{
    Logger::instance().init("horizon-powerd");
    PowerDaemon daemon;
    daemon.run();
    return 0;
}
