#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Label.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/FontSelector.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>
#include <functional>
#include <string>

namespace horizon {
namespace terminal {

class TerminalGeneralSection : public Widget, public ConfigSection {
public:
    TerminalGeneralSection(std::function<void()> on_change) : Widget(), m_on_change(on_change) {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(15);

        auto add_label = [this](const std::string &text) {
            auto label = std::make_unique<Label>(text);
            label->set_font_weight(FONT_WEIGHT_BOLD);
            add_child(std::move(label));
        };

        // 1. Cursor Style
        add_label(i18n().tr("terminal.preferences.cursor_style"));
        auto combo = std::make_unique<Combo>();
        combo->add_item("block", i18n().tr("terminal.preferences.cursor_type_block"));
        combo->add_item("underline", i18n().tr("terminal.preferences.cursor_type_underline"));
        combo->add_item("bar", i18n().tr("terminal.preferences.cursor_type_bar"));
        combo->set_width(200);
        m_cursor_combo = combo.get();
        m_cursor_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(combo));

        // 2. Font & Font Size
        add_label(i18n().tr("core.dialog.font.type_label"));
        auto font_selector = std::make_unique<FontSelector>();
        m_font_selector = font_selector.get();
        m_font_selector->when_font_changed.connect([this](const FontDialogAcceptedContext &) {
            LOG_INFO << "[TERMINAL] Font changed, triggering configuration update";
            if (m_on_change) m_on_change();
        });
        add_child(std::move(font_selector));

        // 4. Scrollback Lines
        add_label(i18n().tr("terminal.preferences.scrollback_lines"));
        auto scrollback_box = std::make_unique<TextBox<IntegerPolicy>>();
        scrollback_box->set_width(150);
        m_scrollback_lines_box = scrollback_box.get();
        m_scrollback_lines_box->when_text_changed.connect([this](const KeyEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(scrollback_box));

        // 5. Checkboxes (Scroll without scrollbar & Show scrollbar)
        auto scroll_check = std::make_unique<Checkbox<AquaObject>>();
        scroll_check->set_text(i18n().tr("terminal.preferences.scroll_without_scrollbar"));
        m_scroll_without_bar_check = scroll_check.get();
        m_scroll_without_bar_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(scroll_check));

        auto show_bar_check = std::make_unique<Checkbox<AquaObject>>();
        show_bar_check->set_text(i18n().tr("terminal.preferences.show_scrollbar"));
        m_show_scrollbar_check = show_bar_check.get();
        m_show_scrollbar_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(show_bar_check));

        auto blink_check = std::make_unique<Checkbox<AquaObject>>();
        blink_check->set_text(i18n().tr("terminal.preferences.cursor_blink"));
        m_cursor_blink_check = blink_check.get();
        m_cursor_blink_check->when_toggle.connect([this](ToggleEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(blink_check));
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        
        if (j.contains("cursor_style")) m_cursor_combo->set_selected_item_by_id(j["cursor_style"].get<std::string>());
        
        FontSelection sel;
        if (j.contains("font")) sel.family = j["font"].get<std::string>();
        if (j.contains("font_size")) sel.size = (float)j["font_size"].get<int>();
        if (j.contains("font_weight")) {
            int weight = j["font_weight"].get<int>();
            if (weight == 1) sel.style = "Bold";
            else sel.style = "Regular";
        }
        m_font_selector->set_selection(sel);

        if (j.contains("scrollback_lines")) m_scrollback_lines_box->set_text(std::to_string(j["scrollback_lines"].get<int>()));
        if (j.contains("scroll_without_scrollbar")) m_scroll_without_bar_check->set_checked(j["scroll_without_scrollbar"].get<bool>());
        if (j.contains("show_scrollbar")) m_show_scrollbar_check->set_checked(j["show_scrollbar"].get<bool>());
        if (j.contains("cursor_blink")) m_cursor_blink_check->set_checked(j["cursor_blink"].get<bool>());
    }

    nlohmann::json to_json() const override {
        nlohmann::json j;
        if (auto selected = m_cursor_combo->selected_item()) {
            j["cursor_style"] = selected->id;
        } else {
            j["cursor_style"] = "block";
        }
        auto sel = m_font_selector->selection();
        j["font"] = sel.family;
        j["font_size"] = (int)sel.size;
        
        int weight = 0;
        if (sel.style.find("Bold") != std::string::npos || sel.style.find("bold") != std::string::npos) {
            weight = 1;
        }
        j["font_weight"] = weight;

        try {
            j["scrollback_lines"] = std::stoi(m_scrollback_lines_box->text());
        } catch (...) { j["scrollback_lines"] = 1000; }

        j["scroll_without_scrollbar"] = m_scroll_without_bar_check->is_checked();
        j["show_scrollbar"] = m_show_scrollbar_check->is_checked();
        j["cursor_blink"] = m_cursor_blink_check->is_checked();
        return j;
    }

private:
    Combo *m_cursor_combo;
    FontSelector *m_font_selector;
    TextBox<IntegerPolicy> *m_scrollback_lines_box;
    Checkbox<AquaObject> *m_scroll_without_bar_check;
    Checkbox<AquaObject> *m_show_scrollbar_check;
    Checkbox<AquaObject> *m_cursor_blink_check;
    std::function<void()> m_on_change;
};

} // namespace terminal
} // namespace horizon
