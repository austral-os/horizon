#include <horizon/Frame.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <utils/ConfigUtils.hpp>
#include <views/AppearanceView/AppearanceView.hpp>
#include <fstream>
#include <cstdlib>

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
        load_compositor_config();
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
        appearance_label->set_fixed_size(200);
        appearance_label->set_alignment(TextAlignment::Right);
        appearance_label->set_vertical_alignment(VerticalAlignment::Top);
        appearance_row->add_child(std::move(appearance_label));

        // Light Theme Box
        auto light_box = std::make_unique<Widget>();
        light_box->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        light_box->set_spacing(10);
        light_box->set_margin(10);
        light_box->set_fixed_size(120);
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
        dark_box->set_fixed_size(120);
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

        appearance_row->add_child(Spacer());

        add_child(std::move(appearance_row));

        // Compositor Section Row
        auto compositor_row = std::make_unique<Widget>();
        compositor_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        compositor_row->set_spacing(20);
        compositor_row->set_fixed_size(120);

        auto compositor_label =
            std::make_unique<Label>(i18n().tr("preferences.appearance.compositor") + ":");
        compositor_label->set_fixed_size(200);
        compositor_label->set_alignment(TextAlignment::Right);
        compositor_label->set_vertical_alignment(VerticalAlignment::Top);
        compositor_row->add_child(std::move(compositor_label));

        // Labwc Card
        auto labwc_box = std::make_unique<Widget>();
        labwc_box->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        labwc_box->set_spacing(10);
        labwc_box->set_margin(10);
        labwc_box->set_fixed_size(120);
        labwc_box->set_border_radius(8);
        labwc_box->set_cursor_type(CursorType::Pointer);

        auto labwc_icon = std::make_unique<Icon>();
        labwc_icon->set_icon_name("window-manager");
        labwc_icon->set_icon_size(80);
        labwc_icon->set_position_type(WidgetPositionTypes::FILL);
        labwc_box->add_child(std::move(labwc_icon));

        auto labwc_label = std::make_unique<Label>(i18n().tr("preferences.appearance.compositor_labwc"));
        labwc_label->set_alignment(TextAlignment::Center);
        labwc_label->set_fixed_size(20);
        labwc_box->add_child(std::move(labwc_label));

        labwc_box->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                set_compositor("labwc");
            });
        m_labwc_box = labwc_box.get();
        compositor_row->add_child(std::move(labwc_box));

        // Wayfire Card
        auto wayfire_box = std::make_unique<Widget>();
        wayfire_box->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        wayfire_box->set_spacing(10);
        wayfire_box->set_margin(10);
        wayfire_box->set_fixed_size(120);
        wayfire_box->set_border_radius(8);
        wayfire_box->set_cursor_type(CursorType::Pointer);

        auto wayfire_icon = std::make_unique<Icon>();
        wayfire_icon->set_icon_name("preferences-desktop");
        wayfire_icon->set_icon_size(80);
        wayfire_icon->set_position_type(WidgetPositionTypes::FILL);
        wayfire_box->add_child(std::move(wayfire_icon));

        auto wayfire_label = std::make_unique<Label>(i18n().tr("preferences.appearance.compositor_wayfire"));
        wayfire_label->set_alignment(TextAlignment::Center);
        wayfire_label->set_fixed_size(20);
        wayfire_box->add_child(std::move(wayfire_label));

        wayfire_box->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                set_compositor("wayfire");
            });
        m_wayfire_box = wayfire_box.get();
        compositor_row->add_child(std::move(wayfire_box));

        compositor_row->add_child(Spacer());
        add_child(std::move(compositor_row));

        // Restart Hint Label Row (UX improvement)
        auto hint_row = std::make_unique<Widget>();
        hint_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        hint_row->set_fixed_size(30);

        auto hint_spacer = std::make_unique<Label>("");
        hint_spacer->set_fixed_size(200);
        hint_row->add_child(std::move(hint_spacer));

        auto hint_label = std::make_unique<Label>("");
        hint_label->set_font_size(11);
        hint_label->set_font_weight(FONT_WEIGHT_NORMAL);
        hint_label->set_alignment(TextAlignment::Left);
        hint_label->set_text_color(Color(0.5f, 0.5f, 0.5f));
        m_restart_hint_label = hint_label.get();
        hint_row->add_child(std::move(hint_label));

        hint_row->add_child(Spacer());
        add_child(std::move(hint_row));

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
        if (m_labwc_box)
        {
            m_labwc_box->set_border_width(m_compositor == "labwc" ? 3 : 1);
            m_labwc_box->set_border_color(m_compositor == "labwc" ? Color(0.12f, 0.3f, 0.88f)
                                                                 : Color(0.5f, 0.5f, 0.5f, 0.3f));
        }
        if (m_wayfire_box)
        {
            m_wayfire_box->set_border_width(m_compositor == "wayfire" ? 3 : 1);
            m_wayfire_box->set_border_color(m_compositor == "wayfire" ? Color(0.12f, 0.3f, 0.88f)
                                                                     : Color(0.5f, 0.5f, 0.5f, 0.3f));
        }
    }

    void AppearanceView::load_compositor_config()
    {
        std::ifstream f("/etc/horizon/compositor.conf");
        if (f.is_open())
        {
            f >> m_compositor;
        }
        else
        {
            m_compositor = "labwc";
        }
    }

    void AppearanceView::set_compositor(const std::string &comp)
    {
        if (comp == m_compositor)
            return;

        // Disparar comando seguro mediante Polkit
        std::string cmd = "pkexec /usr/bin/horizon-set-compositor " + comp;
        int res = std::system(cmd.c_str());

        if (res == 0)
        {
            m_compositor = comp;
            update_selection_visuals();
            if (m_restart_hint_label)
            {
                m_restart_hint_label->set_text(i18n().tr("preferences.appearance.compositor_restart_hint"));
            }
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
