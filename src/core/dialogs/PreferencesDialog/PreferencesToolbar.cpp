#include <horizon/dialogs/PreferencesToolbar.hpp>
#include <horizon/dialogs/PreferencesContent.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Application.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    // --- PreferencesToolbar ---

    PreferencesToolbar::PreferencesToolbar(PreferencesContent *content)
        : m_content(content)
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(4);
        
        for (size_t i = 0; i < m_content->section_count(); ++i)
        {
            auto item = m_content->item_at(i);
            auto button = std::make_unique<ToolbarButton>(item->title(), item->icon());
            button->set_active(i == (size_t)m_active_index);

            button->when_click.connect([this, i](MouseButtonEventContext &) {
                set_active_index(i);
            });

            add_child(std::move(button));
        }
    }

    void PreferencesToolbar::set_active_index(int index)
    {
        m_active_index = index;
        m_content->set_active_section(index);

        // Update active state for all children
        for (size_t i = 0; i < children().size(); ++i) {
            auto *btn = dynamic_cast<ToolbarButton*>(children()[i].get());
            if (btn) {
                btn->set_active(i == (size_t)index);
            }
        }

        invalidate();
    }
} // namespace horizon
