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

    void PreferencesContent::add_section(std::string title, std::string icon, std::unique_ptr<Widget> content)
    {
        content->set_visible(false);
        Widget* content_ptr = content.get();
        m_container->add_child(std::move(content));
        
        m_items.push_back(std::make_unique<PreferencesContentItem>(std::move(title), std::move(icon), content_ptr));
        
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
        return m_config_manager->load();
    }

    bool PreferencesContent::save_config()
    {
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
} // namespace horizon
