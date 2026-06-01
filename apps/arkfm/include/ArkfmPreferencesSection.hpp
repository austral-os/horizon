#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/Label.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/I18n.hpp>
#include <functional>
#include <string>

namespace horizon::arkfm {

class ArkfmPreferencesSection : public Widget, public ConfigSection {
public:
    ArkfmPreferencesSection(std::function<void()> on_change) : Widget(), m_on_change(on_change) {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(15);

        auto add_label = [this](const std::string &text) {
            auto label = std::make_unique<Label>(text);
            label->set_font_weight(FONT_WEIGHT_BOLD);
            label->set_fixed_size(25);
            add_child(std::move(label));
        };

        // Default View
        add_label(i18n().tr("arkfm.preferences.default_view"));
        auto view_combo = std::make_unique<Combo>();
        view_combo->add_item("icon", i18n().tr("arkfm.preferences.view_icon"));
        view_combo->add_item("table", i18n().tr("arkfm.preferences.view_table"));
        view_combo->add_item("coverflow", i18n().tr("arkfm.preferences.view_coverflow"));
        view_combo->set_width(200);
        view_combo->set_fixed_size(35);
        m_default_view_combo = view_combo.get();
        m_default_view_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(view_combo));

        // Click Behavior
        add_label(i18n().tr("arkfm.preferences.click_behavior"));
        auto click_combo = std::make_unique<Combo>();
        click_combo->add_item("double", i18n().tr("arkfm.preferences.double_click"));
        click_combo->add_item("single", i18n().tr("arkfm.preferences.single_click"));
        click_combo->set_width(200);
        click_combo->set_fixed_size(35);
        m_click_behavior_combo = click_combo.get();
        m_click_behavior_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(click_combo));

        // Show Hidden Files
        auto hidden_check = std::make_unique<Checkbox<AquaObject>>();
        hidden_check->set_text(i18n().tr("arkfm.preferences.show_hidden"));
        hidden_check->set_fixed_size(30);
        m_show_hidden_check = hidden_check.get();
        m_show_hidden_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(hidden_check));

        // Show Extensions
        auto ext_check = std::make_unique<Checkbox<AquaObject>>();
        ext_check->set_text(i18n().tr("arkfm.preferences.show_extensions"));
        ext_check->set_fixed_size(30);
        m_show_extensions_check = ext_check.get();
        m_show_extensions_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(ext_check));

        add_child(std::move(Spacer()));
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        
        if (j.contains("default_view")) m_default_view_combo->set_selected_item_by_id(j["default_view"].get<std::string>());
        if (j.contains("click_behavior")) m_click_behavior_combo->set_selected_item_by_id(j["click_behavior"].get<std::string>());
        if (j.contains("show_hidden")) m_show_hidden_check->set_checked(j["show_hidden"].get<bool>());
        if (j.contains("show_extensions")) m_show_extensions_check->set_checked(j["show_extensions"].get<bool>());
    }

    nlohmann::json to_json() const override {
        nlohmann::json j = nlohmann::json::object();
        
        if (auto selected = m_default_view_combo->selected_item()) {
            j["default_view"] = selected->id;
        } else {
            j["default_view"] = "icon";
        }

        if (auto selected = m_click_behavior_combo->selected_item()) {
            j["click_behavior"] = selected->id;
        } else {
            j["click_behavior"] = "double";
        }

        j["show_hidden"] = m_show_hidden_check->is_checked();
        j["show_extensions"] = m_show_extensions_check->is_checked();
        
        return j;
    }

private:
    Combo *m_default_view_combo;
    Combo *m_click_behavior_combo;
    Checkbox<AquaObject> *m_show_hidden_check;
    Checkbox<AquaObject> *m_show_extensions_check;
    std::function<void()> m_on_change;
};

} // namespace horizon::arkfm
