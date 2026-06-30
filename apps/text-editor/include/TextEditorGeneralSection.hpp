#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/Label.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/FontSelector.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/I18n.hpp>
#include <horizon/ThemeManager.hpp>
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

        // 2. Color Scheme — wrapped in a horizontal row so the combo
        //    keeps a reasonable width instead of stretching full-width.
        add_label(i18n().tr("text_editor.preferences.color_scheme"));
        auto combo_row = std::make_unique<Widget>();
        combo_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        combo_row->set_fixed_size(30);
        {
            auto color_scheme_combo = std::make_unique<Combo>();
            color_scheme_combo->set_width(250);
            color_scheme_combo->set_fixed_size(250);
            for (const auto &variant : ThemeManager::instance().app_color_scheme_variants("text-editor")) {
                color_scheme_combo->add_item(variant, variant);
            }
            m_color_scheme_combo = color_scheme_combo.get();
            m_color_scheme_combo->when_item_selected.connect([this](const ComboItemSelectedContext &ctx) {
                if (m_loading) return;
                ThemeManager::instance().set_app_color_scheme_variant("text-editor", ctx.item.id);
                if (m_on_change) m_on_change();
            });
            combo_row->add_child(std::move(color_scheme_combo));
            combo_row->add_child(Spacer());
        }
        add_child(std::move(combo_row));

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

        // 5. Bottom spacer: absorbs extra space when the dialog is resized taller,
        //    keeping controls at the top.
        add_child(Spacer());
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        
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

        m_loading = true;
        std::string variant = j.value("variant", "default");
        if (!ThemeManager::instance().set_app_color_scheme_variant("text-editor", variant)) {
            variant = "default";
            ThemeManager::instance().set_app_color_scheme_variant("text-editor", variant);
        }
        m_color_scheme_combo->set_selected_item_by_id(variant);
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
        if (auto selected = m_color_scheme_combo->selected_item()) {
            j["variant"] = selected->id;
        } else {
            j["variant"] = "default";
        }
        return j;
    }

private:
    Combo *m_color_scheme_combo;
    FontSelector *m_font_selector;
    Checkbox<AquaObject> *m_highlight_check;
    Checkbox<AquaObject> *m_line_numbers_check;
    bool m_loading = false;
    std::function<void()> m_on_change;
};

} // namespace text_editor
} // namespace horizon
