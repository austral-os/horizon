#include <horizon/I18n.hpp>
#include <horizon/Button.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/AquaObject.hpp>
#include <utils/ConfigUtils.hpp>
#include <views/DateTimeView/DateTimeView.hpp>
#include <views/DateTimeView/TimeZonePickerDialog.hpp>
#include <thread>
#include <cstdlib>

namespace horizon::preferences
{
    DateTimeView::DateTimeView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        m_config = std::make_unique<ConfigManager>(get_config_path("datetime.json"));
        m_config->load();

        setup_ui();
        load_config();
    }

    void DateTimeView::setup_ui()
    {
        auto notebook = std::make_unique<Notebook>();
        m_notebook = notebook.get();
        m_notebook->set_position_type(WidgetPositionTypes::FILL);

        // --- Tab 1: Date & Time ---
        auto date_time_page = std::make_unique<Widget>();
        date_time_page->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        date_time_page->set_margin(20);
        date_time_page->set_spacing(15);

        auto auto_update = std::make_unique<Checkbox<AquaObject>>();
        auto_update->set_text(i18n().tr("preferences.datetime.auto_update"));
        m_auto_update_checkbox = auto_update.get();
        m_auto_update_checkbox->when_toggle.connect([this](ToggleEventContext& ctx) {
            apply_system_ntp(ctx.checked);
            save_config();
        });
        date_time_page->add_child(std::move(auto_update));

        m_notebook->add_tab(NotebookPage(i18n().tr("preferences.datetime.tab_date_time"), std::move(date_time_page)));

        // --- Tab 2: Time Zone ---
        auto time_zone_page = std::make_unique<Widget>();
        time_zone_page->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        time_zone_page->set_margin(20);
        time_zone_page->set_spacing(15);

        // Toolbar
        auto toolbar = std::make_unique<Widget>();
        toolbar->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        toolbar->set_fixed_size(35);
        toolbar->set_spacing(10);

        auto btn_add = std::make_unique<Button<AquaObject>>();
        btn_add->set_text(i18n().tr("preferences.datetime.add"));
        btn_add->set_fixed_size(100);
        btn_add->when_click.connect([this](MouseButtonEventContext&) { add_timezone(); });
        toolbar->add_child(std::move(btn_add));

        auto btn_remove = std::make_unique<Button<AquaObject>>();
        btn_remove->set_text(i18n().tr("preferences.datetime.remove"));
        btn_remove->set_fixed_size(100);
        btn_remove->when_click.connect([this](MouseButtonEventContext&) { remove_timezone(); });
        toolbar->add_child(std::move(btn_remove));

        toolbar->add_child(Spacer());

        auto btn_default = std::make_unique<Button<AquaObject>>();
        btn_default->set_text(i18n().tr("preferences.datetime.default"));
        btn_default->set_fixed_size(140);
        btn_default->when_click.connect([this](MouseButtonEventContext&) { set_default_timezone(); });
        toolbar->add_child(std::move(btn_default));

        time_zone_page->add_child(std::move(toolbar));

        // Table
        auto table = std::make_unique<TableView<TimeZoneSelection>>();
        m_timezone_table = table.get();
        m_timezone_table->set_position_type(WidgetPositionTypes::FILL);

        TableColumn<TimeZoneSelection> col_name;
        col_name.title = i18n().tr("preferences.datetime.column_timezone");
        col_name.width = 300;
        col_name.cell_factory = [](const TimeZoneSelection& sel) {
            return std::make_unique<Label>(sel.name);
        };
        m_timezone_table->add_column(col_name);

        TableColumn<TimeZoneSelection> col_def;
        col_def.title = i18n().tr("preferences.datetime.column_default");
        col_def.width = 100;
        col_def.cell_factory = [](const TimeZoneSelection& sel) {
            auto cell = std::make_unique<Widget>();
            cell->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            if (sel.is_default) {
                auto lbl = std::make_unique<Label>(i18n().tr("preferences.datetime.default"));
                lbl->set_font_weight(FONT_WEIGHT_BOLD);
                lbl->set_font_size(12);
                cell->add_child(std::move(lbl));
            }
            return cell;
        };
        m_timezone_table->add_column(col_def);

        time_zone_page->add_child(std::move(table));

        m_notebook->add_tab(NotebookPage(i18n().tr("preferences.datetime.tab_time_zone"), std::move(time_zone_page)));

        add_child(std::move(notebook));
    }

    void DateTimeView::load_config()
    {
        from_json(m_config->get_section("datetime"));
    }

    void DateTimeView::save_config()
    {
        m_config->set_section("datetime", to_json());
        m_config->save();
    }

    void DateTimeView::from_json(const nlohmann::json& j)
    {
        if (j.is_null()) return;

        if (j.contains("auto_update") && m_auto_update_checkbox) {
            m_auto_update_checkbox->set_checked(j["auto_update"].get<bool>());
        }

        if (j.contains("timezones") && j["timezones"].is_array()) {
            m_selected_timezones.clear();
            for (const auto& item : j["timezones"]) {
                TimeZoneSelection sel;
                sel.id = item["id"].get<std::string>();
                sel.name = item["name"].get<std::string>();
                sel.is_default = item.value("default", false);
                m_selected_timezones.push_back(sel);
            }
            m_timezone_table->set_data(m_selected_timezones);
        }
    }

    nlohmann::json DateTimeView::to_json() const
    {
        nlohmann::json j;
        if (m_auto_update_checkbox) {
            j["auto_update"] = m_auto_update_checkbox->is_checked();
        }

        nlohmann::json tzs = nlohmann::json::array();
        for (const auto& sel : m_selected_timezones) {
            nlohmann::json item;
            item["id"] = sel.id;
            item["name"] = sel.name;
            item["default"] = sel.is_default;
            tzs.push_back(item);
        }
        j["timezones"] = tzs;
        return j;
    }

    void DateTimeView::add_timezone()
    {
        auto picker = std::make_unique<TimeZonePickerDialog>();
        auto* picker_ptr = picker.get();

        picker_ptr->when_accepted.connect([this](TimeZone tz) {
            // Check if already exists
            for (const auto& sel : m_selected_timezones) {
                if (sel.id == tz.id) return;
            }

            TimeZoneSelection sel;
            sel.id = tz.id;
            sel.name = tz.name;
            sel.is_default = m_selected_timezones.empty();

            m_selected_timezones.push_back(sel);
            m_timezone_table->set_data(m_selected_timezones);
            
            if (sel.is_default) {
                apply_system_timezone(sel.id);
            }
            
            save_config();
        });

        std::thread([d = std::move(picker)]() mutable {
            d->initialize();
            d->run();
        }).detach();
    }

    void DateTimeView::remove_timezone()
    {
        int idx = m_timezone_table->selected_index();
        if (idx != -1) {
            bool was_default = m_selected_timezones[idx].is_default;
            m_selected_timezones.erase(m_selected_timezones.begin() + idx);

            if (was_default && !m_selected_timezones.empty()) {
                m_selected_timezones[0].is_default = true;
                apply_system_timezone(m_selected_timezones[0].id);
            }

            m_timezone_table->set_data(m_selected_timezones);
            save_config();
        }
    }

    void DateTimeView::set_default_timezone()
    {
        int idx = m_timezone_table->selected_index();
        if (idx != -1) {
            for (auto& sel : m_selected_timezones) {
                sel.is_default = false;
            }
            m_selected_timezones[idx].is_default = true;
            m_timezone_table->set_data(m_selected_timezones);
            
            apply_system_timezone(m_selected_timezones[idx].id);
            save_config();
        }
    }

    void DateTimeView::apply_system_timezone(const std::string& tz_id)
    {
        std::string cmd = "pkexec timedatectl set-timezone " + tz_id;
        std::system(cmd.c_str());
    }

    void DateTimeView::apply_system_ntp(bool enabled)
    {
        std::string cmd = "pkexec timedatectl set-ntp " + std::string(enabled ? "true" : "false");
        std::system(cmd.c_str());
    }
} // namespace horizon::preferences
