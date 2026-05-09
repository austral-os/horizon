#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/TableView.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/ConfigSection.hpp>
#include <utils/TimeZoneUtils.hpp>
#include <memory>
#include <vector>

namespace horizon::preferences
{
    class DateTimeView : public Widget, public ConfigSection
    {
    public:
        DateTimeView();
        ~DateTimeView() override = default;

        // ConfigSection implementation
        void from_json(const nlohmann::json& j) override;
        nlohmann::json to_json() const override;

    private:
        void setup_ui();
        void load_config();
        void save_config();

        void add_timezone();
        void remove_timezone();
        void set_default_timezone();

        void apply_system_timezone(const std::string& tz_id);
        void apply_system_ntp(bool enabled);

        Notebook* m_notebook{nullptr};
        Checkbox<AquaObject>* m_auto_update_checkbox{nullptr};
        TableView<TimeZoneSelection>* m_timezone_table{nullptr};

        std::vector<TimeZoneSelection> m_selected_timezones;
        std::unique_ptr<ConfigManager> m_config;
    };
}
