#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/Label.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/FontSelector.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/I18n.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Application.hpp>
#include <functional>
#include <string>

namespace horizon {
namespace text_editor {

class TextEditorGeneralSection : public Widget, public ConfigSection {
public:
    TextEditorGeneralSection(std::function<void()> on_change) : Widget(), m_on_change(on_change) {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(24);
        set_spacing(15);

        auto add_label = [this](const std::string &text) {
            auto label = std::make_unique<Label>(text);
            label->set_font_weight(FONT_WEIGHT_BOLD);
            label->set_fixed_size(22);
            add_child(std::move(label));
        };

        // 1. Font
        add_label(i18n().tr("core.dialog.font.type_label"));
        auto font_selector = std::make_unique<FontSelector>();
        font_selector->set_fixed_size(36);
        m_font_selector = font_selector.get();
        m_font_selector->when_font_changed.connect([this](const FontDialogAcceptedContext &) {
            if (m_on_change) m_on_change();
        });
        add_child(std::move(font_selector));

        // 2. Language, color scheme, and Tab behavior are grouped in two columns.
        {
            auto settings_row = std::make_unique<Widget>();
            settings_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            settings_row->set_fixed_size(180);
            settings_row->set_spacing(110);

            auto left_settings_column = std::make_unique<Widget>();
            left_settings_column->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            left_settings_column->set_fixed_size(320);
            left_settings_column->set_spacing(8);

            auto language_label = std::make_unique<Label>(i18n().tr("text_editor.preferences.language"));
            language_label->set_font_weight(FONT_WEIGHT_BOLD);
            language_label->set_fixed_size(22);
            left_settings_column->add_child(std::move(language_label));

            auto lang_combo = std::make_unique<Combo>();
            lang_combo->set_width(250);
            lang_combo->set_fixed_size(30);
            lang_combo->add_item("default", i18n().tr("text_editor.preferences.language_default"));
            auto available = i18n().available_app_locales("text-editor");
            for (const auto& loc : available) {
                std::string display = i18n().get_app_locale_display_name("text-editor", loc);
                lang_combo->add_item(loc, display);
            }
            m_language_combo = lang_combo.get();
            m_language_combo->when_item_selected.connect([this](const ComboItemSelectedContext &ctx) {
                if (m_loading) return;
                if (m_on_change) m_on_change();
                if (application()) {
                    application()->alert(
                        i18n().tr("text_editor.preferences.language_restart"),
                        i18n().tr("text_editor.title"),
                        MessageType::Info);
                }
            });
            left_settings_column->add_child(std::move(lang_combo));

            auto color_scheme_label = std::make_unique<Label>(i18n().tr("text_editor.preferences.color_scheme"));
            color_scheme_label->set_font_weight(FONT_WEIGHT_BOLD);
            color_scheme_label->set_fixed_size(22);
            left_settings_column->add_child(std::move(color_scheme_label));

            auto color_scheme_combo = std::make_unique<Combo>();
            color_scheme_combo->set_width(250);
            color_scheme_combo->set_fixed_size(30);
            for (const auto &variant : ThemeManager::instance().app_color_scheme_variants("text-editor")) {
                color_scheme_combo->add_item(variant, variant);
            }
            m_color_scheme_combo = color_scheme_combo.get();
            m_color_scheme_combo->when_item_selected.connect([this](const ComboItemSelectedContext &ctx) {
                if (m_loading) return;
                ThemeManager::instance().set_app_color_scheme_variant("text-editor", ctx.item.id);
                if (m_on_change) m_on_change();
            });
            left_settings_column->add_child(std::move(color_scheme_combo));

            auto tab_settings_column = std::make_unique<Widget>();
            tab_settings_column->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            tab_settings_column->set_fixed_size(320);
            tab_settings_column->set_spacing(8);

            auto tab_behavior_label = std::make_unique<Label>(i18n().tr("text_editor.preferences.tab_behavior"));
            tab_behavior_label->set_font_weight(FONT_WEIGHT_BOLD);
            tab_behavior_label->set_fixed_size(22);
            tab_settings_column->add_child(std::move(tab_behavior_label));

            auto tab_combo = std::make_unique<Combo>();
            tab_combo->set_width(250);
            tab_combo->set_fixed_size(30);
            tab_combo->add_item("tab", i18n().tr("text_editor.preferences.tab_character"));
            tab_combo->add_item("spaces", i18n().tr("text_editor.preferences.tab_spaces"));
            m_tab_mode_combo = tab_combo.get();
            m_tab_mode_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) {
                if (m_loading) return;
                if (m_on_change) m_on_change();
            });
            tab_settings_column->add_child(std::move(tab_combo));

            auto spaces_label = std::make_unique<Label>(i18n().tr("text_editor.preferences.spaces_per_tab"));
            spaces_label->set_font_weight(FONT_WEIGHT_BOLD);
            spaces_label->set_fixed_size(22);
            tab_settings_column->add_child(std::move(spaces_label));

            auto spaces_box = std::make_unique<TextBox<IntegerPolicy>>();
            spaces_box->set_width(120);
            spaces_box->set_fixed_size(30);
            spaces_box->config.min_int = 1;
            spaces_box->config.max_int = 16;
            spaces_box->config.show_spin_buttons = true;
            spaces_box->config.spin_step = 1;
            m_spaces_per_tab_box = spaces_box.get();
            m_spaces_per_tab_box->when_text_changed.connect([this](const KeyEventContext &) {
                if (m_loading) return;
                if (m_on_change) m_on_change();
            });
            tab_settings_column->add_child(std::move(spaces_box));

            settings_row->add_child(std::move(left_settings_column));
            settings_row->add_child(std::move(tab_settings_column));
            settings_row->add_child(Spacer());
            add_child(std::move(settings_row));
        }

        // 3. Highlight Line
        auto highlight_check = std::make_unique<Checkbox<AquaObject>>();
        highlight_check->set_text(i18n().tr("text_editor.preferences.highlight_current_line"));
        m_highlight_check = highlight_check.get();
        m_highlight_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(highlight_check));

        // 4. Line Numbers
        auto line_numbers_check = std::make_unique<Checkbox<AquaObject>>();
        line_numbers_check->set_text(i18n().tr("text_editor.preferences.show_line_numbers"));
        m_line_numbers_check = line_numbers_check.get();
        m_line_numbers_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(line_numbers_check));

        // 5. Highlight Symbols
        auto symbols_check = std::make_unique<Checkbox<AquaObject>>();
        symbols_check->set_text(i18n().tr("text_editor.preferences.highlight_symbols"));
        symbols_check->set_checked(true);
        m_highlight_symbols_check = symbols_check.get();
        m_highlight_symbols_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(symbols_check));

        // 6. Bottom spacer: absorbs extra space when the dialog is resized taller,
        //    keeping controls at the top.
        add_child(Spacer());
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) {
            m_loading = true;
            m_tab_mode_combo->set_selected_item_by_id("tab");
            m_spaces_per_tab_box->set_text("4");
            m_highlight_symbols_check->set_checked(true);
            m_loading = false;
            return;
        }
        
        FontSelection sel;
        if (j.contains("font")) sel.family = j["font"].get<std::string>();
        if (j.contains("font_size")) sel.size = (float)j["font_size"].get<int>();
        if (j.contains("font_weight")) {
            int weight = j["font_weight"].get<int>();
            sel.style = (weight == 1) ? "Bold" : "Regular";
        }
        m_font_selector->set_selection(sel);

        if (j.contains("highlight_line")) m_highlight_check->set_checked(j["highlight_line"].get<bool>());
        if (j.contains("show_line_numbers")) m_line_numbers_check->set_checked(j["show_line_numbers"].get<bool>());
        m_highlight_symbols_check->set_checked(j.value("highlight_symbols", true));

        m_loading = true;

        // Restore language selection (fallback: default)
        m_language_combo->set_selected_item_by_id(j.value("language", "default"));

        // Restore color scheme variant
        std::string variant = j.value("variant", "default");
        if (!ThemeManager::instance().set_app_color_scheme_variant("text-editor", variant)) {
            variant = "default";
            ThemeManager::instance().set_app_color_scheme_variant("text-editor", variant);
        }
        m_color_scheme_combo->set_selected_item_by_id(variant);
        m_tab_mode_combo->set_selected_item_by_id(j.value("tab_mode", "tab"));
        int spaces_per_tab = j.value("spaces_per_tab", 4);
        if (spaces_per_tab < 1) spaces_per_tab = 1;
        if (spaces_per_tab > 16) spaces_per_tab = 16;
        m_spaces_per_tab_box->set_text(std::to_string(spaces_per_tab));
        m_loading = false;
    }

    nlohmann::json to_json() const override {
        nlohmann::json j;
        auto sel = m_font_selector->selection();
        j["font"] = sel.family;
        j["font_size"] = (int)sel.size;
        j["font_weight"] = (sel.style.find("Bold") != std::string::npos || sel.style.find("bold") != std::string::npos) ? 1 : 0;
        j["highlight_line"] = m_highlight_check->is_checked();
        j["show_line_numbers"] = m_line_numbers_check->is_checked();
        j["highlight_symbols"] = m_highlight_symbols_check->is_checked();
        if (auto selected = m_language_combo->selected_item()) {
            j["language"] = selected->id;
        } else {
            j["language"] = "default";
        }
        if (auto selected = m_color_scheme_combo->selected_item()) {
            j["variant"] = selected->id;
        } else {
            j["variant"] = "default";
        }
        if (auto selected = m_tab_mode_combo->selected_item()) {
            j["tab_mode"] = selected->id;
        } else {
            j["tab_mode"] = "tab";
        }
        try {
            int spaces = std::stoi(m_spaces_per_tab_box->text());
            if (spaces < 1) spaces = 1;
            if (spaces > 16) spaces = 16;
            j["spaces_per_tab"] = spaces;
        } catch (...) {
            j["spaces_per_tab"] = 4;
        }
        return j;
    }

private:
    Combo *m_color_scheme_combo;
    Combo *m_language_combo;
    Combo *m_tab_mode_combo;
    FontSelector *m_font_selector;
    TextBox<IntegerPolicy> *m_spaces_per_tab_box;
    Checkbox<AquaObject> *m_highlight_check;
    Checkbox<AquaObject> *m_line_numbers_check;
    Checkbox<AquaObject> *m_highlight_symbols_check;
    bool m_loading = false;
    std::function<void()> m_on_change;
};

} // namespace text_editor
} // namespace horizon
