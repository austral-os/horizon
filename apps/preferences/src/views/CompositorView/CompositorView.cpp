#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <views/CompositorView/CompositorView.hpp>
#include <utils/MeteorPluginManager.hpp>

namespace horizon::preferences
{
    CompositorView::CompositorView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.compositor"));
        title->set_fixed_size(30);
        m_title_label = title.get();
        add_child(std::move(title));

        // Minimize animation (squeezimize)
        auto min_cb = std::make_unique<Checkbox<AquaObject>>();
        m_minimize_cb = min_cb.get();
        m_minimize_cb->set_fixed_size(30);
        m_minimize_cb->set_text(i18n().tr("preferences.compositor.minimize_animation"));
        bool has_min = (MeteorPluginManager::get_config_value("animate", "minimize_animation") == "squeezimize");
        m_minimize_cb->set_checked(has_min);
        m_minimize_cb->when_toggle.connect([](ToggleEventContext& ctx) {
            if (ctx.checked) {
                MeteorPluginManager::set_config_value("animate", "minimize_animation", "squeezimize");
                MeteorPluginManager::set_config_value("animate", "squeezimize_duration", "700ms circle");
            } else {
                MeteorPluginManager::remove_config_value("animate", "minimize_animation");
                MeteorPluginManager::remove_config_value("animate", "squeezimize_duration");
            }
        });
        add_child(std::move(min_cb));

        // Open/Close animation (zoom)
        auto open_cb = std::make_unique<Checkbox<AquaObject>>();
        m_open_close_cb = open_cb.get();
        m_open_close_cb->set_fixed_size(30);
        m_open_close_cb->set_text(i18n().tr("preferences.compositor.open_close_animation"));
        bool has_zoom = (MeteorPluginManager::get_config_value("animate", "open_animation") == "zoom" &&
                         MeteorPluginManager::get_config_value("animate", "close_animation") == "zoom");
        m_open_close_cb->set_checked(has_zoom);
        m_open_close_cb->when_toggle.connect([](ToggleEventContext& ctx) {
            if (ctx.checked) {
                MeteorPluginManager::set_config_value("animate", "open_animation", "zoom");
                MeteorPluginManager::set_config_value("animate", "close_animation", "zoom");
            } else {
                MeteorPluginManager::remove_config_value("animate", "open_animation");
                MeteorPluginManager::remove_config_value("animate", "close_animation");
            }
        });
        add_child(std::move(open_cb));
        setup_checkbox(m_shadows_cb, i18n().tr("preferences.compositor.shadows"), "winshadows");
        setup_checkbox(m_wobbly_cb, i18n().tr("preferences.compositor.wobbly_windows"), "wobbly");
        setup_checkbox(m_blur_cb, i18n().tr("preferences.compositor.blur"), "blur");

        add_child(std::move(Spacer()));
    }

    void CompositorView::setup_checkbox(Checkbox<AquaObject>*& cb, const std::string& label_text, const std::string& plugin_name)
    {
        auto checkbox = std::make_unique<Checkbox<AquaObject>>();
        cb = checkbox.get();
        cb->set_fixed_size(30);
        cb->set_text(label_text);
        
        // Initial state
        cb->set_checked(MeteorPluginManager::is_plugin_enabled(plugin_name));
        
        // Connect toggle event
        cb->when_toggle.connect([plugin_name](ToggleEventContext& ctx) {
            MeteorPluginManager::set_plugin_enabled(plugin_name, ctx.checked);
        });

        add_child(std::move(checkbox));
    }
} // namespace horizon::preferences
