#include <views/RegionView/RegionView.hpp>
#include <horizon/I18n.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <set>
#include <utils/ConfigUtils.hpp>

namespace horizon::preferences
{
    RegionView::RegionView() : Widget()
    {
        m_config = std::make_unique<ConfigManager>(get_config_path("region.json"));
        m_config->load();

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
            m_labels[label_key] = label.get();
            row->add_child(std::move(label));

            auto combo = std::make_unique<Combo>();
            combo_ptr = combo.get();
            row->add_child(std::move(combo));
            this->add_child(std::move(row));
        };

        create_row("preferences.sections.region_language", m_lang_combo);
        create_row("preferences.sections.region_formats", m_formats_combo);
        create_row("preferences.sections.region_timezone", m_timezone_combo);

        load_languages();
        load_formats();
        load_timezones();

        // Restore saved settings
        from_json(m_config->get_section("region"));

        // Connect events after initial load to avoid redundant saves
        auto on_change = [this](const ComboItemSelectedContext&) { save_config(); };
        m_lang_combo->when_item_selected.connect(on_change);
        m_formats_combo->when_item_selected.connect(on_change);
        m_timezone_combo->when_item_selected.connect(on_change);
    }

    void RegionView::load_languages()
    {
        if (!m_lang_combo) return;
        m_lang_combo->clear_items();

        std::set<std::string> seen_codes;
        auto search_paths = I18n::get_search_paths();

        for (const auto& base_path : search_paths) {
            // Find locale files in standard app locations + root
            std::vector<std::string> locales_paths = {
                base_path + "/apps/preferences/locales/",
                base_path + "/locales/"
            };

            for (const auto& path_str : locales_paths) {
                std::filesystem::path locales_path(path_str);
                if (std::filesystem::exists(locales_path) && std::filesystem::is_directory(locales_path)) {
                    for (const auto& entry : std::filesystem::directory_iterator(locales_path)) {
                        if (entry.path().extension() == ".json") {
                            std::string lang_code = entry.path().stem().string();
                            if (lang_code.find("core_") == 0) continue;
                            if (seen_codes.count(lang_code)) continue;

                            // Use core I18n to get the human-readable name
                            std::string display_name = horizon::i18n().get_language_name(lang_code);
                            
                            m_lang_combo->add_item(lang_code, display_name);
                            seen_codes.insert(lang_code);
                        }
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

    void RegionView::from_json(const nlohmann::json& j)
    {
        if (j.is_null()) return;

        if (j.contains("language") && m_lang_combo) {
            m_lang_combo->set_selected_item_by_id(j["language"].get<std::string>());
        }

        if (j.contains("country") && m_formats_combo) {
            m_formats_combo->set_selected_item_by_id(j["country"].get<std::string>());
        }

        if (j.contains("timezone") && m_timezone_combo) {
            m_timezone_combo->set_selected_item_by_id(j["timezone"].get<std::string>());
        }
    }

    nlohmann::json RegionView::to_json() const
    {
        nlohmann::json j;
        if (m_lang_combo && m_lang_combo->selected_item()) {
            j["language"] = m_lang_combo->selected_item()->id;
        }
        if (m_formats_combo && m_formats_combo->selected_item()) {
            j["country"] = m_formats_combo->selected_item()->id;
        }
        if (m_timezone_combo && m_timezone_combo->selected_item()) {
            j["timezone"] = m_timezone_combo->selected_item()->id;
        }
        return j;
    }

    void RegionView::save_config()
    {
        if (m_config) {
            m_config->set_section("region", to_json());
            m_config->save();

            // Apply selected locale to environment and current process
            if (m_lang_combo && m_lang_combo->selected_item()) {
                std::string lang_code = m_lang_combo->selected_item()->id;
                setenv("LANG", lang_code.c_str(), 1);
                horizon::i18n().set_locale(lang_code);
                
                // Refresh UI immediately
                refresh_ui_texts();
            }
        }
    }

    void RegionView::refresh_ui_texts()
    {
        // Reload application strings with new locale
        i18n().load_app_locales("preferences");

        if (m_title_label) {
            m_title_label->set_text(i18n().tr("preferences.sections.region"));
        }

        for (auto const& [key, label] : m_labels) {
            if (label) {
                label->set_text(i18n().tr(key));
            }
        }
    }
}
