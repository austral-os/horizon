#include <views/RegionView/RegionView.hpp>
#include <horizon/I18n.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <set>

namespace horizon::preferences
{
    RegionView::RegionView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.region"));
        title->set_fixed_size(30);
        m_title_label = title.get();
        add_child(std::move(title));

        auto create_row = [this](const std::string& label_key, Combo*& combo_ptr) {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(35);
            row->set_spacing(10);

            auto label = std::make_unique<Label>(i18n().tr(label_key));
            label->set_width(150);
            row->add_child(std::move(label));

            auto combo = std::make_unique<Combo>();
            combo_ptr = combo.get();
            row->add_child(std::move(combo));
            this->add_child(std::move(row));
        };

        create_row("preferences.region_language", m_lang_combo);
        create_row("preferences.region_formats", m_formats_combo);
        create_row("preferences.region_timezone", m_timezone_combo);

        load_languages();
        load_formats();
        load_timezones();
    }

    void RegionView::load_languages()
    {
        if (!m_lang_combo) return;
        m_lang_combo->clear_items();

        std::set<std::string> seen_codes;
        auto search_paths = I18n::get_search_paths();

        for (const auto& base_path : search_paths) {
            std::filesystem::path locales_path = std::filesystem::path(base_path) / "locales";
            if (std::filesystem::exists(locales_path) && std::filesystem::is_directory(locales_path)) {
                for (const auto& entry : std::filesystem::directory_iterator(locales_path)) {
                    if (entry.path().extension() == ".json") {
                        std::string lang_code = entry.path().stem().string();
                        if (lang_code.find("core_") == 0) continue;
                        if (seen_codes.count(lang_code)) continue;

                        std::string display_name = lang_code;
                        try {
                            std::ifstream f(entry.path());
                            auto data = nlohmann::json::parse(f);
                            if (data.contains("language") && data["language"].contains("name")) {
                                display_name = data["language"]["name"].get<std::string>();
                            }
                        } catch (...) {}

                        m_lang_combo->add_item(lang_code, display_name);
                        seen_codes.insert(lang_code);
                    }
                }
            }
        }
    }

    void RegionView::load_formats()
    {
        if (!m_formats_combo) return;
        m_formats_combo->clear_items();

        // Standard paths for countries.json
        std::vector<std::string> paths = {
            "/usr/share/horizon/countries.json",
            "data/countries.json", // Local dev fallback
            "../apps/preferences/data/countries.json" // Relative dev fallback
        };

        std::string found_path;
        for (const auto& p : paths) {
            if (std::filesystem::exists(p)) {
                found_path = p;
                break;
            }
        }

        if (found_path.empty()) return;

        try {
            std::ifstream f(found_path);
            auto data = nlohmann::json::parse(f);
            if (data.contains("countries") && data["countries"].is_array()) {
                for (const auto& item : data["countries"]) {
                    std::string id = item.value("id", "");
                    std::string name = item.value("name", id);
                    m_formats_combo->add_item(id, name);
                }
            }
        } catch (...) {}
    }

    void RegionView::load_timezones()
    {
        if (!m_timezone_combo) return;
        m_timezone_combo->clear_items();

        FILE* pipe = popen("timedatectl list-timezones", "r");
        if (!pipe) return;

        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string tz(buffer);
            if (!tz.empty() && tz.back() == '\n') tz.pop_back();
            if (!tz.empty()) {
                m_timezone_combo->add_item(tz, tz);
            }
        }
        pclose(pipe);
    }
}
