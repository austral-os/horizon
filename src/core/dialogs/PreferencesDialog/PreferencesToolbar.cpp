#include <horizon/dialogs/PreferencesToolbar.hpp>
#include <horizon/dialogs/PreferencesContent.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Application.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{
    // --- PreferencesToolbarButton ---

    PreferencesToolbarButton::PreferencesToolbarButton(std::string title, std::string icon, int index, PreferencesToolbar *toolbar)
        : m_title(std::move(title)), m_icon_name(std::move(icon)), m_index(index), m_toolbar(toolbar)
    {
        set_fixed_size(64); // Standard width for these buttons
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_spacing(0);
        
        auto icon_widget = std::make_unique<Icon>();
        icon_widget->set_icon_name(m_icon_name);
        icon_widget->set_icon_size(32);
        icon_widget->set_margin(4);
        add_child(std::move(icon_widget));

        auto label_widget = std::make_unique<Label>(m_title);
        label_widget->set_font_size(10);
        label_widget->set_alignment(TextAlignment::Center);
        label_widget->set_height(12);
        add_child(std::move(label_widget));

        when_click.connect([this](MouseButtonEventContext &) {
            m_toolbar->set_active_index(m_index);
        });
    }

    void PreferencesToolbarButton::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        bool is_active = m_toolbar->active_index() == m_index;

        if (is_active || m_is_hovered)
        {
            Color highlight = tm->get_color("titlebar_bg2");
            if (is_active) {
                highlight.a = 0.4f;
            } else {
                highlight.a = 0.2f;
            }
            gc.setColor(highlight);
            gc.fillRect(m_start_draw_x + 2, m_start_draw_y + 2, m_width - 4, m_height - 4, 6);
        }
    }

    // --- PreferencesToolbar ---

    PreferencesToolbar::PreferencesToolbar(PreferencesContent *content)
        : m_content(content)
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(4);
        
        for (size_t i = 0; i < m_content->section_count(); ++i)
        {
            auto item = m_content->item_at(i);
            auto button = std::make_unique<PreferencesToolbarButton>(item->title(), item->icon(), i, this);
            add_child(std::move(button));
        }
    }

    void PreferencesToolbar::set_active_index(int index)
    {
        m_active_index = index;
        m_content->set_active_section(index);
        invalidate();
    }
} // namespace horizon
