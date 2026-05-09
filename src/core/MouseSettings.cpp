#include <horizon/MouseSettings.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/Logger.hpp>
#include <filesystem>
#include <cstdlib>

namespace horizon
{
    MouseSettings &MouseSettings::instance()
    {
        static MouseSettings inst;
        return inst;
    }

    MouseSettings::MouseSettings()
    {
        const char *home = std::getenv("HOME");
        if (home)
        {
            std::filesystem::path p(home);
            p /= ".config/horizon/mouse.json";
            m_config_path = p.string();
        }

        load();

        if (!m_config_path.empty())
        {
            start_watching(m_config_path);
        }
    }

    MouseSettings::~MouseSettings()
    {
        stop_watching();
    }

    int MouseSettings::double_click_speed() const
    {
        return m_double_click_speed.load();
    }

    void MouseSettings::on_file_changed()
    {
        LOG_INFO << "[MouseSettings] Configuration change detected, reloading...";
        load();
    }

    void MouseSettings::post_watcher_task(std::function<void()> task)
    {
        // Since we are just reloading atomic values, we don't strictly need 
        // to be on the main thread. We execute the task (which calls on_file_changed)
        // directly from the watcher thread.
        if (task) task();
    }

    void MouseSettings::load()
    {
        if (m_config_path.empty() || !std::filesystem::exists(m_config_path))
        {
            m_double_click_speed = 250;
            return;
        }

        try
        {
            ConfigManager config(m_config_path);
            if (config.load())
            {
                auto j = config.get_section("mouse");
                if (j.contains("double_click_speed"))
                {
                    m_double_click_speed = j["double_click_speed"].get<int>();
                    LOG_INFO << "[MouseSettings] Double click speed loaded: " << m_double_click_speed.load() << "ms";
                    return;
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "[MouseSettings] Error loading config: " << e.what();
        }

        m_double_click_speed = 250;
    }
} // namespace horizon
