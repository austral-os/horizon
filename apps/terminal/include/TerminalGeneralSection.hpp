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
        add_label("Estilo del Cursor:");
        auto combo = std::make_unique<Combo>();
        combo->add_item("block", "Bloque");
        combo->add_item("underline", "Subrayado");
        combo->add_item("bar", "Barra");
        combo->set_width(200);
        m_cursor_combo = combo.get();
        m_cursor_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(combo));

        // 2. Font
        add_label("Fuente:");
        auto font_box = std::make_unique<TextBox<>>();
        font_box->set_placeholder("Ej: CaskaydiaCove Nerd Font Mono");
        font_box->set_width(350);
        m_font_box = font_box.get();
        m_font_box->when_text_changed.connect([this](const KeyEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(font_box));

        // 3. Font Size
        add_label("Tamaño de Fuente:");
        auto size_box = std::make_unique<TextBox<IntegerPolicy>>();
        size_box->set_width(100);
        m_font_size_box = size_box.get();
        m_font_size_box->when_text_changed.connect([this](const KeyEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(size_box));

        // 4. Scrollback Lines
        add_label("Líneas de Scrollback:");
        auto scrollback_box = std::make_unique<TextBox<IntegerPolicy>>();
        scrollback_box->set_width(150);
        m_scrollback_lines_box = scrollback_box.get();
        m_scrollback_lines_box->when_text_changed.connect([this](const KeyEventContext &) { if (m_on_change) m_on_change(); });
        add_child(std::move(scrollback_box));

        // 5. Checkboxes (Scroll without scrollbar & Show scrollbar)
        auto scroll_check = std::make_unique<Checkbox<AquaObject>>();
        scroll_check->set_text("Scroll sin barra de desplazamiento");
        m_scroll_without_bar_check = scroll_check.get();
        m_scroll_without_bar_check->set_on_toggle([this](bool) { if (m_on_change) m_on_change(); });
        add_child(std::move(scroll_check));

        auto show_bar_check = std::make_unique<Checkbox<AquaObject>>();
        show_bar_check->set_text("Mostrar barra de desplazamiento");
        m_show_scrollbar_check = show_bar_check.get();
        m_show_scrollbar_check->set_on_toggle([this](bool) { if (m_on_change) m_on_change(); });
        add_child(std::move(show_bar_check));
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        
        if (j.contains("cursor_style")) m_cursor_combo->set_selected_item_by_id(j["cursor_style"].get<std::string>());
        if (j.contains("font")) m_font_box->set_text(j["font"].get<std::string>());
        if (j.contains("font_size")) m_font_size_box->set_text(std::to_string(j["font_size"].get<int>()));
        if (j.contains("scrollback_lines")) m_scrollback_lines_box->set_text(std::to_string(j["scrollback_lines"].get<int>()));
        if (j.contains("scroll_without_scrollbar")) m_scroll_without_bar_check->set_checked(j["scroll_without_scrollbar"].get<bool>());
        if (j.contains("show_scrollbar")) m_show_scrollbar_check->set_checked(j["show_scrollbar"].get<bool>());
    }

    nlohmann::json to_json() const override {
        nlohmann::json j;
        if (auto selected = m_cursor_combo->selected_item()) {
            j["cursor_style"] = selected->id;
        } else {
            j["cursor_style"] = "block";
        }
        j["font"] = m_font_box->text();
        
        try {
            j["font_size"] = std::stoi(m_font_size_box->text());
        } catch (...) { j["font_size"] = 12; }

        try {
            j["scrollback_lines"] = std::stoi(m_scrollback_lines_box->text());
        } catch (...) { j["scrollback_lines"] = 1000; }

        j["scroll_without_scrollbar"] = m_scroll_without_bar_check->is_checked();
        j["show_scrollbar"] = m_show_scrollbar_check->is_checked();
        return j;
    }

private:
    Combo *m_cursor_combo;
    TextBox<TextPolicy> *m_font_box;
    TextBox<IntegerPolicy> *m_font_size_box;
    TextBox<IntegerPolicy> *m_scrollback_lines_box;
    Checkbox<AquaObject> *m_scroll_without_bar_check;
    Checkbox<AquaObject> *m_show_scrollbar_check;
    std::function<void()> m_on_change;
};

} // namespace terminal
} // namespace horizon
