#include <horizon/Frame.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <utils/ConfigUtils.hpp>
#include <views/AppearanceView/AppearanceView.hpp>

namespace horizon::preferences
{
    AppearanceView::AppearanceView() : Widget()
    {
        m_is_loading = true;
        m_config = std::make_unique<ConfigManager>(get_config_path("color-scheme.json"));
        m_config->load();

        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(20);

        from_json(m_config->get_section("variant"));
        setup_ui();
        m_is_loading = false;
    }

    void AppearanceView::setup_ui()
    {
        clear_children();

        // Title
        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.appearance"));
        title->set_font_size(18);
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_fixed_size(40);
        m_title_label = title.get();
        add_child(std::move(title));

        // Appearance Section Row
        auto appearance_row = std::make_unique<Widget>();
        appearance_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        appearance_row->set_spacing(20);
        appearance_row->set_fixed_size(120);

        auto appearance_label =
            std::make_unique<Label>(i18n().tr("preferences.appearance.title") + ":");
        appearance_label->set_fixed_size(150);
        appearance_row->add_child(std::move(appearance_label));

        // Light Theme Box
        auto light_box = std::make_unique<Widget>();
        light_box->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        light_box->set_spacing(10);
        light_box->set_margin(10);
        light_box->set_width(100);
        light_box->set_border_radius(8);
        light_box->set_cursor_type(CursorType::Pointer);

        auto light_icon = std::make_unique<Icon>();
        light_icon->set_icon_name("theme-variant-light");
        light_icon->set_icon_size(80);
        light_icon->set_position_type(WidgetPositionTypes::FILL);
        light_box->add_child(std::move(light_icon));

        auto light_label = std::make_unique<Label>(i18n().tr("preferences.appearance.light"));
        light_label->set_alignment(TextAlignment::Center);
        light_label->set_fixed_size(20);
        light_box->add_child(std::move(light_label));

        light_box->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                m_variant = "light";
                save_config();
                update_selection_visuals();
            });
        m_light_box = light_box.get();
        appearance_row->add_child(std::move(light_box));

        // Dark Theme Box
        auto dark_box = std::make_unique<Widget>();
        dark_box->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        dark_box->set_spacing(10);
        dark_box->set_margin(10);
        dark_box->set_width(100);
        dark_box->set_border_radius(8);
        dark_box->set_cursor_type(CursorType::Pointer);

        auto dark_icon = std::make_unique<Icon>();
        dark_icon->set_icon_name("theme-variant-dark");
        dark_icon->set_icon_size(80);
        dark_icon->set_position_type(WidgetPositionTypes::FILL);
        dark_box->add_child(std::move(dark_icon));

        auto dark_label = std::make_unique<Label>(i18n().tr("preferences.appearance.dark"));
        dark_label->set_alignment(TextAlignment::Center);
        dark_label->set_fixed_size(20);
        dark_box->add_child(std::move(dark_label));

        dark_box->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                m_variant = "dark";
                save_config();
                update_selection_visuals();
            });
        m_dark_box = dark_box.get();
        appearance_row->add_child(std::move(dark_box));

        add_child(std::move(appearance_row));

        update_selection_visuals();
    }

    void AppearanceView::update_selection_visuals()
    {
        if (m_light_box)
        {
            m_light_box->set_border_width(m_variant == "light" ? 3 : 1);
            m_light_box->set_border_color(m_variant == "light" ? Color(0.12f, 0.3f, 0.88f)
                                                               : Color(0.5f, 0.5f, 0.5f, 0.3f));
        }
        if (m_dark_box)
        {
            m_dark_box->set_border_width(m_variant == "dark" ? 3 : 1);
            m_dark_box->set_border_color(m_variant == "dark" ? Color(0.12f, 0.3f, 0.88f)
                                                             : Color(0.5f, 0.5f, 0.5f, 0.3f));
        }
    }

    void AppearanceView::from_json(const nlohmann::json &j)
    {
        if (j.is_string())
        {
            m_variant = j.get<std::string>();
        }
    }

    nlohmann::json AppearanceView::to_json() const
    {
        return m_variant;
    }

    void AppearanceView::save_config()
    {
        if (m_is_loading)
            return;
        m_config->set_section("variant", to_json());
        m_config->save();
    }
} // namespace horizon::preferences
