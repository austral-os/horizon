#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/Label.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/FontSelector.hpp>
#include <horizon/I18n.hpp>
#include <functional>
#include <string>

namespace horizon {
namespace text_editor {

class TextEditorGeneralSection : public Widget, public ConfigSection {
public:
    TextEditorGeneralSection(std::function<void()> on_change) : Widget(), m_on_change(on_change) {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(15);

        auto add_label = [this](const std::string &text) {
            auto label = std::make_unique<Label>(text);
            label->set_font_weight(FONT_WEIGHT_BOLD);
            add_child(std::move(label));
        };

        // 1. Font
        add_label(i18n().tr("core.dialog.font.type_label"));
        auto font_selector = std::make_unique<FontSelector>();
        m_font_selector = font_selector.get();
        m_font_selector->when_font_changed.connect([this](const FontDialogAcceptedContext &) {
            if (m_on_change) m_on_change();
        });
        add_child(std::move(font_selector));

        // 2. Highlight Line
        auto highlight_check = std::make_unique<Checkbox<AquaObject>>();
        highlight_check->set_text(i18n().tr("text_editor.preferences.highlight_current_line"));
        m_highlight_check = highlight_check.get();
        m_highlight_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(highlight_check));

        // 3. Line Numbers
        auto line_numbers_check = std::make_unique<Checkbox<AquaObject>>();
        line_numbers_check->set_text(i18n().tr("text_editor.preferences.show_line_numbers"));
        m_line_numbers_check = line_numbers_check.get();
        m_line_numbers_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(line_numbers_check));
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
    }

    nlohmann::json to_json() const override {
        nlohmann::json j;
        auto sel = m_font_selector->selection();
        j["font"] = sel.family;
        j["font_size"] = (int)sel.size;
        j["font_weight"] = (sel.style.find("Bold") != std::string::npos || sel.style.find("bold") != std::string::npos) ? 1 : 0;
        j["highlight_line"] = m_highlight_check->is_checked();
        j["show_line_numbers"] = m_line_numbers_check->is_checked();
        return j;
    }

private:
    FontSelector *m_font_selector;
    Checkbox<AquaObject> *m_highlight_check;
    Checkbox<AquaObject> *m_line_numbers_check;
    std::function<void()> m_on_change;
};

} // namespace text_editor
} // namespace horizon
