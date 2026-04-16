#include <horizon/dialogs/PreferencesContent.hpp>

namespace horizon
{
    PreferencesContent::PreferencesContent(const std::string &config_path) 
        : Widget()
    {
        m_config_manager = std::make_unique<ConfigManager>(config_path);
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        
        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_position_type(WidgetPositionTypes::FILL);
        m_container = container.get();
        
        add_child(std::move(container));

        // Load configuration on startup
        load_config();
    }

    void PreferencesContentItem::from_json(const nlohmann::json &j)
    {
        if (auto section = dynamic_cast<ConfigSection *>(m_content))
        {
            section->from_json(j);
        }
    }

    nlohmann::json PreferencesContentItem::to_json() const
    {
        if (auto section = dynamic_cast<ConfigSection *>(m_content))
        {
            return section->to_json();
        }
        return nlohmann::json();
    }

    void PreferencesContent::add_section(std::string title, std::string icon, std::unique_ptr<Widget> content, std::string section_name)
    {
        if (section_name.empty())
        {
            section_name = slugify(title);
        }

        content->set_visible(false);
        Widget *content_ptr = content.get();
        m_container->add_child(std::move(content));

        auto item = std::make_unique<PreferencesContentItem>(std::move(title), std::move(icon), std::move(section_name), content_ptr);
        
        // Sync the item with current config data if available
        item->from_json(m_config_manager->get_section(item->section_name()));

        m_items.push_back(std::move(item));

        if (m_items.size() == 1)
        {
            set_active_section(0);
        }
    }

    void PreferencesContent::set_active_section(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_items.size()))
            return;

        if (m_active_index >= 0 && m_active_index < static_cast<int>(m_items.size()))
        {
            m_items[m_active_index]->content()->set_visible(false);
        }

        m_active_index = index;
        m_items[m_active_index]->content()->set_visible(true);
        m_items[m_active_index]->content()->invalidate();
        
        invalidate();
    }

    PreferencesContentItem *PreferencesContent::item_at(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_items.size()))
            return nullptr;
        return m_items[index].get();
    }

    // --- Configuration Persistence ---

    bool PreferencesContent::load_config()
    {
        bool success = m_config_manager->load();
        if (success)
        {
            for (auto &item : m_items)
            {
                item->from_json(m_config_manager->get_section(item->section_name()));
            }
        }
        return success;
    }

    bool PreferencesContent::save_config()
    {
        for (auto &item : m_items)
        {
            nlohmann::json section_data = item->to_json();
            if (!section_data.is_null())
            {
                // We merge the new data into the existing section to avoid overwriting 
                // properties managed by other sections sharing the same name.
                nlohmann::json existing = m_config_manager->get_section(item->section_name());
                existing.update(section_data);
                m_config_manager->set_section(item->section_name(), existing);
            }
        }
        return m_config_manager->save();
    }

    void PreferencesContent::set_config_value(const std::string &section, const std::string &key, const nlohmann::json &value)
    {
        m_config_manager->set_value(section, key, value);
    }

    nlohmann::json PreferencesContent::get_config_value(const std::string &section, const std::string &key, const nlohmann::json &default_value)
    {
        return m_config_manager->get_value(section, key, default_value);
    }

    std::string PreferencesContent::slugify(const std::string &text)
    {
        std::string result;
        bool last_was_dash = false;

        for (char c : text)
        {
            if (std::isalnum(c))
            {
                result += static_cast<char>(std::tolower(c));
                last_was_dash = false;
            }
            else if (!last_was_dash && !result.empty())
            {
                result += '-';
                last_was_dash = true;
            }
        }

        // Remove trailing dash
        if (!result.empty() && result.back() == '-')
        {
            result.pop_back();
        }

        return result;
    }
} // namespace horizon
