#include <algorithm>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/Window.hpp>
#include <horizon/I18n.hpp>
#include <views/DateTimeView/TimeZonePickerDialog.hpp>

namespace horizon::preferences
{
    TimeZonePickerDialog::TimeZonePickerDialog() : WaylandWindow("horizon.timezone_picker", 450, 550, true, true)
    {
        set_name(i18n().tr("preferences.datetime.select_timezone"));
        setup_ui();
        load_timezones();
    }

    void TimeZonePickerDialog::setup_ui()
    {
        auto root_wnd = std::make_unique<Window>(name());
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root_wnd->set_margin(0);
        root_wnd->set_spacing(0);

        auto container = std::make_unique<Widget>();
        container->set_margin(15);
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(10);

        // --- Search Area ---
        auto search_box = std::make_unique<SearchBox>();
        search_box->set_placeholder(i18n().tr("preferences.datetime.search_timezones"));
        search_box->set_fixed_size(35);
        m_search_box = search_box.get();
        m_search_box->when_text_changed.connect([this](KeyEventContext &)
                                                { filter_timezones(m_search_box->text()); });
        container->add_child(std::move(search_box));

        // --- Table Area ---
        auto table = std::make_unique<TableView<TimeZone>>();
        m_timezone_table = table.get();
        m_timezone_table->set_header_visible(false);
        m_timezone_table->set_position_type(WidgetPositionTypes::FILL);

        TableColumn<TimeZone> col;
        col.width = 400;
        col.cell_factory = [](const TimeZone &tz)
        {
            auto cell = std::make_unique<Widget>();
            cell->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            cell->set_spacing(10);

            auto label = std::make_unique<Label>(tz.name);
            label->set_font_size(14);
            cell->add_child(std::move(label));

            return cell;
        };
        m_timezone_table->add_column(col);
        container->add_child(std::move(table));

        // --- Footer Area ---
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(35);
        buttons->set_spacing(10);

        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text(i18n().tr("preferences.datetime.cancel"));
        btn_cancel->set_fixed_size(100);
        btn_cancel->when_click.connect([this](MouseButtonEventContext &) { this->quit(); });
        buttons->add_child(std::move(btn_cancel));

        auto btn_accept = std::make_unique<Button<AquaObject>>();
        btn_accept->set_text(i18n().tr("preferences.datetime.accept"));
        btn_accept->set_fixed_size(100);
        btn_accept->set_accent_color(WidgetAccentColor::Primary);
        btn_accept->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                int idx = m_timezone_table->selected_index();
                if (idx != -1)
                {
                    when_accepted.run(m_filtered_timezones[idx]);
                    this->quit();
                }
            });
        buttons->add_child(std::move(btn_accept));

        container->add_child(std::move(buttons));

        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));
    }

    void TimeZonePickerDialog::load_timezones()
    {
        m_all_timezones = TimeZoneUtils::get_all_timezones();

        // Sort by name
        std::sort(m_all_timezones.begin(), m_all_timezones.end(),
                  [](const TimeZone &a, const TimeZone &b) { return a.name < b.name; });

        m_filtered_timezones = m_all_timezones;
        m_timezone_table->set_data(m_filtered_timezones);
    }

    void TimeZonePickerDialog::filter_timezones(const std::string &query)
    {
        if (query.empty())
        {
            m_filtered_timezones = m_all_timezones;
        }
        else
        {
            m_filtered_timezones.clear();
            std::string lower_query = query;
            std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

            for (const auto &tz : m_all_timezones)
            {
                std::string lower_name = tz.name;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

                if (lower_name.find(lower_query) != std::string::npos)
                {
                    m_filtered_timezones.push_back(tz);
                }
            }
        }
        m_timezone_table->set_data(m_filtered_timezones);
    }
} // namespace horizon::preferences
