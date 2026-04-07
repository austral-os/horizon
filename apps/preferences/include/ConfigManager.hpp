#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace horizon::preferences
{
    /**
     * @class ConfigManager
     * @brief Singleton manager for handling application configuration in JSON format.
     */
    class ConfigManager
    {
    public:
        static ConfigManager& instance();

        /**
         * @brief Load the configuration file from ~/.config/horizon/horizon.json.
         * @return true if loaded successfully or file doesn't exist (initial state), false on error.
         */
        bool load();

        /**
         * @brief Save the current configuration back to disk.
         * @return true if saved successfully, false on error.
         */
        bool save();

        /**
         * @brief Get the JSON data for a specific section.
         * @param name The name of the section (e.g., "desktop").
         * @return nlohmann::json The section data, or an empty object if not found.
         */
        nlohmann::json get_section(const std::string& name) const;

        /**
         * @brief Update the JSON data for a specific section.
         * @param name The name of the section.
         * @param data The new JSON data for the section.
         */
        void set_section(const std::string& name, const nlohmann::json& data);

    private:
        ConfigManager() = default;
        ~ConfigManager() = default;
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;

        std::string get_config_path() const;
        bool ensure_config_dir() const;

    private:
        nlohmann::json m_config_data;
    };
} // namespace horizon::preferences
