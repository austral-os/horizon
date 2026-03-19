#include "ConfigManager.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>

namespace horizon::preferences
{
    ConfigManager& ConfigManager::instance()
    {
        static ConfigManager instance;
        return instance;
    }

    bool ConfigManager::load()
    {
        std::string path = get_config_path();
        if (path.empty()) return false;

        if (!std::filesystem::exists(path))
        {
            m_config_data = nlohmann::json::object();
            return true;
        }

        try
        {
            std::ifstream file(path);
            if (!file.is_open()) return false;
            file >> m_config_data;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ConfigManager] Error loading config: " << e.what() << std::endl;
            m_config_data = nlohmann::json::object();
            return false;
        }
    }

    bool ConfigManager::save()
    {
        if (!ensure_config_dir()) return false;

        std::string path = get_config_path();
        if (path.empty()) return false;

        try
        {
            std::ofstream file(path);
            if (!file.is_open()) return false;
            file << m_config_data.dump(4);
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ConfigManager] Error saving config: " << e.what() << std::endl;
            return false;
        }
    }

    nlohmann::json ConfigManager::get_section(const std::string& name) const
    {
        if (m_config_data.contains(name))
        {
            return m_config_data[name];
        }
        return nlohmann::json::object();
    }

    void ConfigManager::set_section(const std::string& name, const nlohmann::json& data)
    {
        m_config_data[name] = data;
        save();
    }

    std::string ConfigManager::get_config_path() const
    {
        const char* home = std::getenv("HOME");
        if (!home) return "";

        std::filesystem::path config_path(home);
        config_path /= ".config/horizon/horizon.json";
        return config_path.string();
    }

    bool ConfigManager::ensure_config_dir() const
    {
        const char* home = std::getenv("HOME");
        if (!home) return false;

        std::filesystem::path config_dir(home);
        config_dir /= ".config/horizon";

        if (!std::filesystem::exists(config_dir))
        {
            try
            {
                return std::filesystem::create_directories(config_dir);
            }
            catch (const std::exception& e)
            {
                std::cerr << "[ConfigManager] Error creating config directory: " << e.what() << std::endl;
                return false;
            }
        }
        return true;
    }
} // namespace horizon::preferences
