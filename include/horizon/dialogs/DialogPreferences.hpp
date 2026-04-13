#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace horizon
{
    class PreferencesContent;
    class DialogPreferences : public WaylandWindow
    {
    public:
        DialogPreferences(const std::string &title, const std::string &config_filename, int width = 600, int height = 400, bool defer_init = false);
        ~DialogPreferences() override = default;

        /**
         * @brief Returns the internal toolbar.
         */
        Toolbar *toolbar() const;

        /**
         * @brief Sets the content widget of the dialog.
         */
        void set_content(std::unique_ptr<Widget> content);

        /**
         * @brief Returns the content widget of the dialog.
         */
        Widget *content() const;

        /**
         * @brief Sets up a PreferencesToolbar for the given content.
         */
        void setup_toolbar(PreferencesContent *content);

        // --- Configuration Persistence ---

        /**
         * @brief Loads the configuration from ~/.config/horizon/<config_filename>.
         * @return true if successful.
         */
        bool load_config();

        /**
         * @brief Saves the current configuration to disk.
         * @return true if successful.
         */
        bool save_config();

        /**
         * @brief Sets a configuration value for a specific section.
         */
        void set_config_value(const std::string &section, const std::string &key, const nlohmann::json &value);

        /**
         * @brief Gets a configuration value from a specific section.
         */
        nlohmann::json get_config_value(const std::string &section, const std::string &key, const nlohmann::json &default_value = nlohmann::json());

        /**
         * @brief Access to the underlying configuration object.
         */
        nlohmann::json &config_data() { return m_config_data; }

    private:
        std::string get_config_path() const;
        bool ensure_config_dir() const;

        ApplicationWindow *m_app_window{nullptr};
        std::string m_config_filename;
        nlohmann::json m_config_data;
    };
} // namespace horizon
