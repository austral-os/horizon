#include "RegionPage.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Label.hpp>
#include <horizon/Combo.hpp>
#include <horizon/ToolbarButton.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <iostream>

namespace horizon::installer
{
    RegionPage::RegionPage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto title = std::make_unique<Label>(i18n().tr("installer.region.title"));
        title->set_font_size(32);
        title->set_fixed_size(60);
        add_child(std::move(title));

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_margin(20);

        // Country
        auto country_label = std::make_unique<Label>(i18n().tr("installer.region.formats") + ":");
        country_label->set_fixed_size(25);
        content->add_child(std::move(country_label));

        auto country_combo = std::make_unique<Combo>();
        country_combo->set_height(35);
        auto* country_ptr = country_combo.get();
        
        // Load countries
        std::string countries_path = "/usr/share/horizon/countries.json";
        if (!std::filesystem::exists(countries_path)) {
             char result[4096];
             ssize_t count = readlink("/proc/self/exe", result, 4096);
             if (count != -1) {
                 std::string path(result, count);
                 std::string dir = path.substr(0, path.find_last_of('/'));
                 countries_path = dir + "/../apps/horizon_session/data/countries.json";
             }
        }

        if (std::filesystem::exists(countries_path)) {
            try {
                std::ifstream f(countries_path);
                nlohmann::json data = nlohmann::json::parse(f);
                for (auto& item : data) {
                    std::string code = item["code"];
                    std::string name = item["name"];
                    country_ptr->add_item(code, name);
                }
            } catch(...) {}
        }
        
        if (auto* item = country_ptr->selected_item()) {
            m_selected_country = item->id;
        }

        country_ptr->when_item_selected.connect([this, country_ptr](const auto& ctx) {
            m_selected_country = ctx.item.id;
        });

        content->add_child(std::move(country_combo));
        content->add_child(Spacer(20));

        // Timezone
        auto tz_label = std::make_unique<Label>(i18n().tr("preferences.sections.region_timezone") + ":");
        tz_label->set_fixed_size(25);
        content->add_child(std::move(tz_label));

        auto tz_combo = std::make_unique<Combo>();
        tz_combo->set_height(35);
        auto* tz_ptr = tz_combo.get();

        FILE* pipe = popen("timedatectl list-timezones", "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string tz = buffer;
                if (!tz.empty() && tz.back() == '\n') tz.pop_back();
                tz_ptr->add_item(tz, tz);
            }
            pclose(pipe);
        }

        if (auto* item = tz_ptr->selected_item()) {
            m_selected_timezone = item->id;
        }

        tz_ptr->when_item_selected.connect([this, tz_ptr](const auto& ctx) {
            m_selected_timezone = ctx.item.id;
        });

        content->add_child(std::move(tz_combo));
        add_child(std::move(content));

        add_child(Spacer());

        auto footer = std::make_unique<Widget>();
        footer->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        footer->set_height(60);

        auto back_btn = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.back"), "go-previous");
        back_btn->when_click.connect([this](auto&) { 
            EventContext ctx;
            when_back.run(ctx); 
        });
        footer->add_child(std::move(back_btn));

        footer->add_child(Spacer());

        auto continue_btn = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.continue"), "go-next");
        continue_btn->when_click.connect([this](auto&) { 
            EventContext ctx;
            when_continue.run(ctx); 
        });
        footer->add_child(std::move(continue_btn));

        add_child(std::move(footer));
    }
} // namespace horizon::installer
