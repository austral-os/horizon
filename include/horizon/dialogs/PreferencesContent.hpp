#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ConfigManager.hpp>
#include <string>
#include <memory>
#include <vector>

namespace horizon
{
    /**
     * @brief Represents a section in the PreferencesContent widget.
     */
    class PreferencesContentItem
    {
    public:
        PreferencesContentItem(std::string title, std::string icon, Widget *content)
            : m_title(std::move(title)), m_icon(std::move(icon)), m_content(content)
        {
        }

        const std::string &title() const { return m_title; }
        const std::string &icon() const { return m_icon; }
        Widget *content() const { return m_content; }

    private:
        std::string m_title;
        std::string m_icon;
        Widget *m_content;
    };

    /**
     * @brief A widget that manages multiple preference sections, showing only one at a time.
     */
    class PreferencesContent : public Widget
    {
    public:
        PreferencesContent(const std::string &config_path);
        ~PreferencesContent() override = default;

        /**
         * @brief Adds a new section to the preferences content.
         */
        void add_section(std::string title, std::string icon, std::unique_ptr<Widget> content);

        /**
         * @brief Sets the active section by index.
         */
        void set_active_section(int index);

        /**
         * @brief Returns the index of the current active section.
         */
        int active_section_index() const { return m_active_index; }

        /**
         * @brief Returns the number of sections.
         */
        size_t section_count() const { return m_items.size(); }

        /**
         * @brief Returns the item at the given index.
         */
        PreferencesContentItem *item_at(int index);

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
        nlohmann::json &config_data() { return m_config_manager->config_data(); }

        std::vector<std::unique_ptr<PreferencesContentItem>> m_items;
        int m_active_index{-1};
        Widget *m_container{nullptr};

        std::unique_ptr<ConfigManager> m_config_manager;
    };
} // namespace horizon
