#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/Label.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/ColorSelector.hpp>
#include <horizon/Slider.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <functional>
#include <vector>

namespace horizon {
namespace terminal {

class TerminalColorSection : public Widget, public ConfigSection {
public:
    TerminalColorSection(std::function<void()> on_change) : Widget(), m_on_change(on_change) {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(15);

        auto add_title = [this](const std::string &key) {
            auto label = std::make_unique<Label>(i18n().tr(key));
            label->set_font_weight(FONT_WEIGHT_BOLD);
            add_child(std::move(label));
        };

        auto create_row = [](int spacing = 10) {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_spacing(spacing);
            return row;
        };

        // --- Text and Background Color ---
        add_title("terminal.preferences.colors.title");

        auto system_theme_check = std::make_unique<Checkbox<AquaObject>>();
        system_theme_check->set_text(i18n().tr("terminal.preferences.colors.use_system_theme"));
        m_use_system_theme_check = system_theme_check.get();
        m_use_system_theme_check->set_on_toggle([this](bool) { if (m_on_change) m_on_change(); });
        add_child(std::move(system_theme_check));

        auto scheme_row = create_row();
        auto scheme_lbl = std::make_unique<Label>(i18n().tr("terminal.preferences.colors.builtin_schemes"));
        scheme_lbl->set_width(150);
        scheme_row->add_child(std::move(scheme_lbl));
        auto scheme_combo = std::make_unique<Combo>();
        scheme_combo->add_item("tango_dark", "Tango dark");
        scheme_combo->add_item("tango_light", "Tango light");
        scheme_combo->set_width(200);
        m_scheme_combo = scheme_combo.get();
        m_scheme_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) { if (m_on_change) m_on_change(); });
        scheme_row->add_child(std::move(scheme_combo));
        add_child(std::move(scheme_row));

        // Grid for colors
        // Labels row
        auto labels_row = create_row();
        auto empty_lbl = std::make_unique<Label>("");
        empty_lbl->set_width(150);
        labels_row->add_child(std::move(empty_lbl));
        auto text_lbl = std::make_unique<Label>(i18n().tr("terminal.preferences.colors.text"));
        text_lbl->set_width(80);
        text_lbl->set_alignment(TextAlignment::Center);
        labels_row->add_child(std::move(text_lbl));
        auto bg_lbl = std::make_unique<Label>(i18n().tr("terminal.preferences.colors.background"));
        bg_lbl->set_width(80);
        bg_lbl->set_alignment(TextAlignment::Center);
        labels_row->add_child(std::move(bg_lbl));
        add_child(std::move(labels_row));

        // Default color
        auto default_row = create_row();
        auto default_lbl = std::make_unique<Label>(i18n().tr("terminal.preferences.colors.default_color"));
        default_lbl->set_width(150);
        default_row->add_child(std::move(default_lbl));
        auto fg_sel = std::make_unique<ColorSelector>();
        fg_sel->set_width(80);
        m_fg_color = fg_sel.get();
        m_fg_color->when_color_changed.connect([this](const ColorPickerDialogAcceptedContext &) { if (m_on_change) m_on_change(); });
        default_row->add_child(std::move(fg_sel));
        auto bg_sel = std::make_unique<ColorSelector>();
        bg_sel->set_width(80);
        m_bg_color = bg_sel.get();
        m_bg_color->when_color_changed.connect([this](const ColorPickerDialogAcceptedContext &) { if (m_on_change) m_on_change(); });
        default_row->add_child(std::move(bg_sel));
        add_child(std::move(default_row));

        // Bold, Cursor, Highlight rows with checkboxes
        auto add_color_row = [&](const std::string &label_key, ColorSelector **fg, ColorSelector **bg, Checkbox<AquaObject> **check) {
            auto row = create_row();
            auto c = std::make_unique<Checkbox<AquaObject>>();
            c->set_text(i18n().tr(label_key));
            c->set_width(150);
            *check = c.get();
            (*check)->set_on_toggle([this](bool) { if (m_on_change) m_on_change(); });
            row->add_child(std::move(c));
            
            auto f = std::make_unique<ColorSelector>();
            f->set_width(80);
            *fg = f.get();
            (*fg)->when_color_changed.connect([this](const ColorPickerDialogAcceptedContext &) { if (m_on_change) m_on_change(); });
            row->add_child(std::move(f));
            
            if (bg) {
                auto b = std::make_unique<ColorSelector>();
                b->set_width(80);
                *bg = b.get();
                (*bg)->when_color_changed.connect([this](const ColorPickerDialogAcceptedContext &) { if (m_on_change) m_on_change(); });
                row->add_child(std::move(b));
            } else {
                auto sp = std::make_unique<Widget>();
                sp->set_width(80);
                row->add_child(std::move(sp));
            }
            add_child(std::move(row));
        };

        add_color_row("terminal.preferences.colors.bold_color", &m_bold_fg_color, nullptr, &m_bold_color_check);
        add_color_row("terminal.preferences.colors.cursor_color", &m_cursor_fg_color, &m_cursor_bg_color, &m_cursor_color_check);
        add_color_row("terminal.preferences.colors.highlight_color", &m_highlight_fg_color, &m_highlight_bg_color, &m_highlight_color_check);

        // Transparency
        auto trans_row = create_row();
        auto trans_check = std::make_unique<Checkbox<AquaObject>>();
        trans_check->set_text(i18n().tr("terminal.preferences.colors.use_transparent_bg"));
        trans_check->set_width(220);
        m_use_transparency_check = trans_check.get();
        m_use_transparency_check->set_on_toggle([this](bool) { if (m_on_change) m_on_change(); });
        trans_row->add_child(std::move(trans_check));
        
        auto lbl_none = std::make_unique<Label>(i18n().tr("terminal.preferences.colors.transparency_none"));
        trans_row->add_child(std::move(lbl_none));
        
        auto slider = std::make_unique<Slider>();
        slider->set_width(200);
        m_transparency_slider = slider.get();
        m_transparency_slider->when_changed.connect([this](const EventContext &) { if (m_on_change) m_on_change(); });
        trans_row->add_child(std::move(slider));
        
        auto lbl_full = std::make_unique<Label>(i18n().tr("terminal.preferences.colors.transparency_full"));
        trans_row->add_child(std::move(lbl_full));
        add_child(std::move(trans_row));

        auto system_trans_check = std::make_unique<Checkbox<AquaObject>>();
        system_trans_check->set_text(i18n().tr("terminal.preferences.colors.use_system_transparency"));
        m_use_system_transparency_check = system_trans_check.get();
        m_use_system_transparency_check->set_on_toggle([this](bool) { if (m_on_change) m_on_change(); });
        add_child(std::move(system_trans_check));

        // --- Palette ---
        add_child(Spacer());
        add_title("terminal.preferences.colors.palette_title");

        auto palette_scheme_row = create_row();
        auto palette_scheme_lbl = std::make_unique<Label>(i18n().tr("terminal.preferences.colors.builtin_schemes"));
        palette_scheme_lbl->set_width(150);
        palette_scheme_row->add_child(std::move(palette_scheme_lbl));
        auto palette_combo = std::make_unique<Combo>();
        palette_combo->add_item("tango", "Tango");
        palette_combo->set_width(200);
        m_palette_combo = palette_combo.get();
        m_palette_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) { if (m_on_change) m_on_change(); });
        palette_scheme_row->add_child(std::move(palette_combo));
        add_child(std::move(palette_scheme_row));

        auto palette_row = create_row();
        auto palette_lbl = std::make_unique<Label>(i18n().tr("terminal.preferences.colors.color_palette"));
        palette_lbl->set_width(150);
        palette_row->add_child(std::move(palette_lbl));
        
        auto palette_grid = std::make_unique<Widget>();
        palette_grid->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        palette_grid->set_spacing(5);
        
        auto palette_grid_row1 = create_row(5);
        auto palette_grid_row2 = create_row(5);
        
        for (int i = 0; i < 8; ++i) {
            auto s1 = std::make_unique<ColorSelector>();
            s1->set_width(40);
            m_palette[i] = s1.get();
            m_palette[i]->when_color_changed.connect([this](const ColorPickerDialogAcceptedContext &) { if (m_on_change) m_on_change(); });
            palette_grid_row1->add_child(std::move(s1));
            
            auto s2 = std::make_unique<ColorSelector>();
            s2->set_width(40);
            m_palette[i+8] = s2.get();
            m_palette[i+8]->when_color_changed.connect([this](const ColorPickerDialogAcceptedContext &) { if (m_on_change) m_on_change(); });
            palette_grid_row2->add_child(std::move(s2));
        }
        palette_grid->add_child(std::move(palette_grid_row1));
        palette_grid->add_child(std::move(palette_grid_row2));
        palette_row->add_child(std::move(palette_grid));
        add_child(std::move(palette_row));

        auto bold_bright_check = std::make_unique<Checkbox<AquaObject>>();
        bold_bright_check->set_text(i18n().tr("terminal.preferences.colors.show_bold_bright"));
        m_show_bold_bright_check = bold_bright_check.get();
        m_show_bold_bright_check->set_on_toggle([this](bool) { if (m_on_change) m_on_change(); });
        add_child(std::move(bold_bright_check));
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        
        auto get_color = [](const nlohmann::json &parent, const std::string &key, ColorSelector *sel) {
            if (parent.contains(key)) {
                auto c = parent[key];
                if (c.is_array() && c.size() >= 3) {
                    sel->set_color(Color(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c.size() > 3 ? c[3].get<float>() : 1.0f));
                }
            }
        };

        if (j.contains("use_system_theme")) m_use_system_theme_check->set_checked(j["use_system_theme"].get<bool>());
        if (j.contains("scheme")) m_scheme_combo->set_selected_item_by_id(j["scheme"].get<std::string>());
        
        get_color(j, "foreground_color", m_fg_color);
        get_color(j, "background_color", m_bg_color);
        
        if (j.contains("bold_color_configured")) m_bold_color_check->set_checked(j["bold_color_configured"].get<bool>());
        get_color(j, "bold_color", m_bold_fg_color);
        
        if (j.contains("cursor_colors_configured")) m_cursor_color_check->set_checked(j["cursor_colors_configured"].get<bool>());
        get_color(j, "cursor_fg_color", m_cursor_fg_color);
        get_color(j, "cursor_bg_color", m_cursor_bg_color);

        if (j.contains("highlight_colors_configured")) m_highlight_color_check->set_checked(j["highlight_colors_configured"].get<bool>());
        get_color(j, "highlight_fg_color", m_highlight_fg_color);
        get_color(j, "highlight_bg_color", m_highlight_bg_color);

        if (j.contains("use_transparent_bg")) m_use_transparency_check->set_checked(j["use_transparent_bg"].get<bool>());
        if (j.contains("transparency")) m_transparency_slider->set_value(j["transparency"].get<float>());
        if (j.contains("use_system_transparency")) m_use_system_transparency_check->set_checked(j["use_system_transparency"].get<bool>());

        if (j.contains("palette_scheme")) m_palette_combo->set_selected_item_by_id(j["palette_scheme"].get<std::string>());
        if (j.contains("palette") && j["palette"].is_array()) {
            auto p = j["palette"];
            for (size_t i = 0; i < 16 && i < p.size(); ++i) {
                auto c = p[i];
                if (c.is_array() && c.size() >= 3) {
                    m_palette[i]->set_color(Color(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c.size() > 3 ? c[3].get<float>() : 1.0f));
                }
            }
        }
        if (j.contains("show_bold_bright")) m_show_bold_bright_check->set_checked(j["show_bold_bright"].get<bool>());
    }

    nlohmann::json to_json() const override {
        nlohmann::json j;
        
        auto color_to_json = [](const Color &c) {
            return nlohmann::json{c.r, c.g, c.b, c.a};
        };

        j["use_system_theme"] = m_use_system_theme_check->is_checked();
        if (auto sel = m_scheme_combo->selected_item()) j["scheme"] = sel->id;
        
        j["foreground_color"] = color_to_json(m_fg_color->color());
        j["background_color"] = color_to_json(m_bg_color->color());
        
        j["bold_color_configured"] = m_bold_color_check->is_checked();
        j["bold_color"] = color_to_json(m_bold_fg_color->color());
        
        j["cursor_colors_configured"] = m_cursor_color_check->is_checked();
        j["cursor_fg_color"] = color_to_json(m_cursor_fg_color->color());
        j["cursor_bg_color"] = color_to_json(m_cursor_bg_color->color());

        j["highlight_colors_configured"] = m_highlight_color_check->is_checked();
        j["highlight_fg_color"] = color_to_json(m_highlight_fg_color->color());
        j["highlight_bg_color"] = color_to_json(m_highlight_bg_color->color());

        j["use_transparent_bg"] = m_use_transparency_check->is_checked();
        j["transparency"] = m_transparency_slider->value();
        j["use_system_transparency"] = m_use_system_transparency_check->is_checked();

        if (auto sel = m_palette_combo->selected_item()) j["palette_scheme"] = sel->id;
        
        nlohmann::json p = nlohmann::json::array();
        for (int i = 0; i < 16; ++i) {
            p.push_back(color_to_json(m_palette[i]->color()));
        }
        j["palette"] = p;
        j["show_bold_bright"] = m_show_bold_bright_check->is_checked();
        
        return j;
    }

private:
    Checkbox<AquaObject> *m_use_system_theme_check;
    Combo *m_scheme_combo;
    ColorSelector *m_fg_color;
    ColorSelector *m_bg_color;
    Checkbox<AquaObject> *m_bold_color_check;
    ColorSelector *m_bold_fg_color;
    Checkbox<AquaObject> *m_cursor_color_check;
    ColorSelector *m_cursor_fg_color;
    ColorSelector *m_cursor_bg_color;
    Checkbox<AquaObject> *m_highlight_color_check;
    ColorSelector *m_highlight_fg_color;
    ColorSelector *m_highlight_bg_color;
    Checkbox<AquaObject> *m_use_transparency_check;
    Slider *m_transparency_slider;
    Checkbox<AquaObject> *m_use_system_transparency_check;
    Combo *m_palette_combo;
    ColorSelector *m_palette[16];
    Checkbox<AquaObject> *m_show_bold_bright_check;
    std::function<void()> m_on_change;
};

} // namespace terminal
} // namespace horizon
