#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace horizon
{
    /**
     * @class ConfigManager
     * @brief Manager for handling application configuration in JSON format.
     */
    class ConfigManager
    {
    public:
        /**
         * @brief Construct a ConfigManager for a specific configuration file.
         * @param config_path The absolute path to the configuration file.
         */
        ConfigManager(const std::string& config_path);
        ~ConfigManager() = default;

        /**
         * @brief Load the configuration file.
         * @return true if loaded successfully, false on error.
         */
        bool load();

        /**
         * @brief Save the current configuration back to disk.
         * @return true if saved successfully, false on error.
         */
        bool save();

        /**
         * @brief Get the JSON data for a specific section.
         * @param name The name of the section.
         * @return nlohmann::json The section data, or an empty object if not found.
         */
        nlohmann::json get_section(const std::string& name) const;

        /**
         * @brief Update the JSON data for a specific section.
         * @param name The name of the section.
         * @param data The new JSON data for the section.
         */
        void set_section(const std::string& name, const nlohmann::json& data);

        /**
         * @brief Get a specific value within a section.
         */
        nlohmann::json get_value(const std::string& section, const std::string& key, const nlohmann::json& default_value = nlohmann::json()) const;

        /**
         * @brief Set a specific value within a section.
         */
        void set_value(const std::string& section, const std::string& key, const nlohmann::json& value);

        /**
         * @brief Access to the underlying configuration object.
         */
        nlohmann::json& config_data() { return m_config_data; }
        const nlohmann::json& config_data() const { return m_config_data; }

    private:
        bool ensure_config_dir() const;

        std::string m_config_path;
        nlohmann::json m_config_data;
    };
} // namespace horizon
