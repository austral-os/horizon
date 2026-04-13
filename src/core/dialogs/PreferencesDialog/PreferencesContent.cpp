#include "PreferencesContent.hpp"

namespace horizon
{
    PreferencesContent::PreferencesContent() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        
        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_position_type(WidgetPositionTypes::FILL);
        m_container = container.get();
        
        add_child(std::move(container));
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
} // namespace horizon
