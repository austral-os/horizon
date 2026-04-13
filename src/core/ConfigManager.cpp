#include <horizon/ConfigManager.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace horizon
{
    ConfigManager::ConfigManager(const std::string& config_path)
        : m_config_path(config_path)
    {
    }

    bool ConfigManager::load()
    {
        if (!ensure_config_dir()) return false;

        if (!std::filesystem::exists(m_config_path))
        {
            m_config_data = nlohmann::json::object();
            // Create the file immediately if it doesn't exist
            return save();
        }

        try
        {
            std::ifstream file(m_config_path);
            if (!file.is_open()) return false;
            file >> m_config_data;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ConfigManager] Error loading config from " << m_config_path << ": " << e.what() << std::endl;
            m_config_data = nlohmann::json::object();
            return false;
        }
    }

    bool ConfigManager::save()
    {
        if (!ensure_config_dir()) return false;

        try
        {
            std::ofstream file(m_config_path);
            if (!file.is_open()) return false;
            file << m_config_data.dump(4);
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ConfigManager] Error saving config to " << m_config_path << ": " << e.what() << std::endl;
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

    nlohmann::json ConfigManager::get_value(const std::string& section, const std::string& key, const nlohmann::json& default_value) const
    {
        if (m_config_data.contains(section) && m_config_data[section].contains(key))
        {
            return m_config_data[section][key];
        }
        return default_value;
    }

    void ConfigManager::set_value(const std::string& section, const std::string& key, const nlohmann::json& value)
    {
        if (!m_config_data.contains(section))
        {
            m_config_data[section] = nlohmann::json::object();
        }
        m_config_data[section][key] = value;
        // Optimization: we could decide not to save on every set_value, 
        // but for now let's keep it consistent with set_section.
        save();
    }

    bool ConfigManager::ensure_config_dir() const
    {
        std::filesystem::path p(m_config_path);
        std::filesystem::path dir = p.parent_path();

        if (!dir.empty() && !std::filesystem::exists(dir))
        {
            try
            {
                return std::filesystem::create_directories(dir);
            }
            catch (const std::exception& e)
            {
                std::cerr << "[ConfigManager] Error creating directory " << dir << ": " << e.what() << std::endl;
                return false;
            }
        }
        return true;
    }
} // namespace horizon
