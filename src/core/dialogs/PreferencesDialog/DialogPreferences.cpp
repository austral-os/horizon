#include <horizon/dialogs/DialogPreferences.hpp>
#include <horizon/dialogs/PreferencesToolbar.hpp>
#include <horizon/Toolbar.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>

namespace horizon
{
    DialogPreferences::DialogPreferences(const std::string &title, const std::string &config_filename, int width, int height, bool defer_init)
        : WaylandWindow("horizon.dialog.preferences", width, height, defer_init, true), m_config_filename(config_filename)
    {
        set_name(title);

        auto app_window = std::make_unique<ApplicationWindow>(title);
        m_app_window = app_window.get();
        set_root(std::move(app_window));

        // Load configuration on startup
        load_config();
    }

    Toolbar *DialogPreferences::toolbar() const
    {
        return m_app_window ? m_app_window->toolbar() : nullptr;
    }

    void DialogPreferences::set_content(std::unique_ptr<Widget> content)
    {
        if (m_app_window)
        {
            m_app_window->set_content(std::move(content));
        }
    }

    Widget *DialogPreferences::content() const
    {
        return m_app_window ? m_app_window->content() : nullptr;
    }

    void DialogPreferences::setup_toolbar(PreferencesContent *content)
    {
        if (m_app_window && content)
        {
            auto toolbar_widget = std::make_unique<PreferencesToolbar>(content);
            m_app_window->toolbar()->add_toolbar_widget(std::move(toolbar_widget));
        }
    }

    // --- Configuration Persistence ---

    bool DialogPreferences::load_config()
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
        catch (const std::exception &e)
        {
            std::cerr << "[DialogPreferences] Error loading config: " << e.what() << std::endl;
            m_config_data = nlohmann::json::object();
            return false;
        }
    }

    bool DialogPreferences::save_config()
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
        catch (const std::exception &e)
        {
            std::cerr << "[DialogPreferences] Error saving config: " << e.what() << std::endl;
            return false;
        }
    }

    void DialogPreferences::set_config_value(const std::string &section, const std::string &key, const nlohmann::json &value)
    {
        if (!m_config_data.contains(section))
        {
            m_config_data[section] = nlohmann::json::object();
        }
        m_config_data[section][key] = value;
    }

    nlohmann::json DialogPreferences::get_config_value(const std::string &section, const std::string &key, const nlohmann::json &default_value)
    {
        if (m_config_data.contains(section) && m_config_data[section].contains(key))
        {
            return m_config_data[section][key];
        }
        return default_value;
    }

    std::string DialogPreferences::get_config_path() const
    {
        const char *home = std::getenv("HOME");
        if (!home) return "";

        std::filesystem::path config_path(home);
        config_path /= ".config/horizon";
        config_path /= m_config_filename;
        return config_path.string();
    }

    bool DialogPreferences::ensure_config_dir() const
    {
        const char *home = std::getenv("HOME");
        if (!home) return false;

        std::filesystem::path config_dir(home);
        config_dir /= ".config/horizon";

        if (!std::filesystem::exists(config_dir))
        {
            try
            {
                return std::filesystem::create_directories(config_dir);
            }
            catch (const std::exception &e)
            {
                std::cerr << "[DialogPreferences] Error creating config directory: " << e.what() << std::endl;
                return false;
            }
        }
        return true;
    }
} // namespace horizon
