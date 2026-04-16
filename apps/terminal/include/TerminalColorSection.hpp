#pragma once

#include "TerminalColorScheme.hpp"
#include "horizon/AquaObject.hpp"
#include "horizon/Notebook.hpp"
#include <filesystem>
#include <functional>
#include <horizon/Checkbox.hpp>
#include <horizon/ColorSelector.hpp>
#include <horizon/Combo.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/Slider.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>
#include <vector>

namespace horizon
{
    namespace terminal
    {

        class TerminalColorSection : public Widget, public ConfigSection
        {
        public:
            TerminalColorSection(std::function<void()> on_change) : Widget(), m_on_change(on_change)
            {
                set_layout_type(WIDGET_LAYOUT_VERTICAL);
                set_margin(0);
                set_spacing(0);

                // --- Load Themes (Multi-directory discovery) ---
                m_themes = TerminalColorScheme::list_available_themes();

                auto nb = std::make_unique<Notebook>();

                auto general_page = std::make_unique<Widget>();
                general_page->set_layout_type(WIDGET_LAYOUT_VERTICAL);
                general_page->set_spacing(15);
                general_page->set_margin(15);

                // --- Theme Selection ---
                auto combo_theme = std::make_unique<Combo>();
                m_theme_combo = combo_theme.get();
                m_theme_combo->set_fixed_size(37);
                for (const auto &th : m_themes)
                {
                    m_theme_combo->add_item(th.name, th.name);
                }
                m_theme_combo->when_item_selected.connect(
                    [this](const ComboItemSelectedContext &ctx)
                    {
                        for (const auto &th : m_themes)
                        {
                            if (th.name == ctx.item.id)
                            {
                                m_current_theme = th;
                                sync_selectors_from_theme();
                                break;
                            }
                        }
                        if (m_on_change)
                            m_on_change();
                    });

                auto chk_sys_theme = std::make_unique<Checkbox<AquaObject>>();
                m_chk_sys_theme = chk_sys_theme.get();
                chk_sys_theme->set_text(i18n().tr("terminal.preferences.colors.use_system_theme"));
                chk_sys_theme->when_toggle.connect(
                    [this](ToggleEventContext &)
                    {
                        if (m_on_change)
                            m_on_change();
                    });

                auto lbl_opacity = std::make_unique<horizon::Label>(
                    i18n().tr("terminal.preferences.colors.use_transparent_bg"));
                lbl_opacity->set_fixed_size(25);
                lbl_opacity->set_font_weight(FONT_WEIGHT_BOLD);

                auto row_slider = std::make_unique<Widget>();
                row_slider->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
                row_slider->set_fixed_size(25);

                auto lbl_transparency = std::make_unique<horizon::Label>("0%");
                lbl_transparency->set_fixed_size(60);
                m_transparency_label = lbl_transparency.get();

                auto slider = std::make_unique<horizon::Slider>();
                slider->set_orientation(horizon::SliderOrientation::Horizontal);
                slider->set_show_ticks(true);
                slider->set_min(0.0f);
                slider->set_max(100.0f);
                slider->set_value(100.0f);
                m_transparency_slider = slider.get();
                m_transparency_slider->add_custom_tick(25.0f);
                m_transparency_slider->add_custom_tick(50.0f);
                m_transparency_slider->add_custom_tick(75.0f);
                m_transparency_slider->add_custom_tick(95.0f);

                m_transparency_slider->when_changed.connect([this](const horizon::EventContext &)
                                                            { m_on_change(); });

                m_transparency_slider->when_value_changed.connect(
                    [this](const horizon::EventContext &)
                    {
                        float val = m_transparency_slider->value();
                        m_transparency = static_cast<int>(val);
                        m_transparency_label->set_text(std::to_string(m_transparency) + "%");
                    });

                row_slider->add_child(std::move(slider));
                row_slider->add_child(std::move(lbl_transparency));

                // --- Primary Colors ---
                auto lbl_primary = std::make_unique<Label>("Colores Principales:");
                lbl_primary->set_font_weight(FONT_WEIGHT_BOLD);
                lbl_primary->set_fixed_size(25);

                auto row_primary = std::make_unique<Widget>();
                row_primary->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
                row_primary->set_spacing(10);
                row_primary->set_fixed_size(35);

                auto create_primary_selector = [this](const std::string &label_text,
                                                      ColorSelector *&ptr,
                                                      std::string &target_field)
                {
                    auto container = std::make_unique<Widget>();
                    container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
                    container->set_spacing(5);

                    auto lbl = std::make_unique<Label>(label_text);
                    lbl->set_margin(5);
                    lbl->set_fixed_size(160);
                    lbl->set_alignment(TextAlignment::Right);

                    auto sel = std::make_unique<ColorSelector>();
                    sel->set_fixed_size(60);
                    ptr = sel.get();

                    sel->when_color_changed.connect(
                        [this, &target_field](const ColorPickerDialogAcceptedContext &ctx)
                        {
                            target_field = ctx.color.to_hex();
                            ensure_custom_theme();
                            if (m_on_change)
                                m_on_change();
                        });

                    container->add_child(std::move(lbl));
                    container->add_child(std::move(sel));
                    return container;
                };

                auto sel_bg = create_primary_selector("Fondo:", m_bg_selector,
                                                      m_current_theme.primary.background);
                auto sel_fg = create_primary_selector("Texto:", m_fg_selector,
                                                      m_current_theme.primary.foreground);
                auto sel_cursor = create_primary_selector("Cursor:", m_cursor_selector,
                                                          m_current_theme.primary.cursor);

                row_primary->add_child(std::move(sel_bg));
                row_primary->add_child(std::move(sel_fg));
                row_primary->add_child(std::move(sel_cursor));
                row_primary->add_child(Spacer());

                auto lbl_palette = std::make_unique<Label>("Paleta de Colores");
                lbl_palette->set_font_weight(FONT_WEIGHT_BOLD);
                lbl_palette->set_fixed_size(25);

                // --- Color Palette Rows ---
                auto create_palette_row =
                    [this](std::vector<ColorSelector *> &selectors, bool is_bright)
                {
                    auto row = std::make_unique<Widget>();
                    row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
                    row->set_spacing(10);
                    row->set_fixed_size(35);

                    for (int i = 0; i < 8; ++i)
                    {
                        auto selector = std::make_unique<ColorSelector>();
                        selector->set_fixed_size(60);
                        selectors.push_back(selector.get());

                        // Connect color change event
                        selectors.back()->when_color_changed.connect(
                            [this, i, is_bright](const ColorPickerDialogAcceptedContext &ctx)
                            {
                                std::string hex = ctx.color.to_hex();
                                if (is_bright)
                                {
                                    auto colors = m_current_theme.bright.to_vector();
                                    colors[i] = hex;
                                    m_current_theme.bright.from_vector(colors);
                                }
                                else
                                {
                                    auto colors = m_current_theme.normal.to_vector();
                                    colors[i] = hex;
                                    m_current_theme.normal.from_vector(colors);
                                }

                                ensure_custom_theme();
                                if (m_on_change)
                                    m_on_change();
                            });

                        row->add_child(std::move(selector));
                    }
                    return row;
                };

                auto lbl_normal = std::make_unique<Label>("Normal:");
                lbl_normal->set_margin(2);
                lbl_normal->set_fixed_size(25);
                auto row_normal = create_palette_row(m_normal_selectors, false);

                auto lbl_bright = std::make_unique<Label>("Intenso:");
                lbl_bright->set_fixed_size(25);
                auto row_bright = create_palette_row(m_bright_selectors, true);

                general_page->add_child(std::move(combo_theme));
                general_page->add_child(std::move(chk_sys_theme));
                general_page->add_child(std::move(row_slider));
                general_page->add_child(std::move(lbl_primary));
                general_page->add_child(std::move(row_primary));
                general_page->add_child(std::move(lbl_palette));
                general_page->add_child(std::move(lbl_normal));
                general_page->add_child(std::move(row_normal));
                general_page->add_child(std::move(lbl_bright));
                general_page->add_child(std::move(row_bright));

                // Initial sync
                sync_selectors_from_theme();

                nb->add_tab(NotebookPage("General", std::move(general_page)));
                nb->add_tab(NotebookPage("Paleta", std::make_unique<Widget>()));

                add_child(std::move(nb));
            }

            void from_json(const nlohmann::json &j) override
            {
                if (j.is_null())
                    return;

                m_transparency = j.value("transparency", 100);
                m_transparency_label->set_text(std::to_string(m_transparency) + "%");

                if (j.contains("use_system_theme"))
                    m_chk_sys_theme->set_checked(j["use_system_theme"].get<bool>());

                if (m_transparency_slider)
                    m_transparency_slider->set_value(static_cast<float>(m_transparency));
            }

            void set_current_theme(const TerminalColorScheme &theme)
            {
                m_current_theme = theme;

                // Ensure the theme is in the list and combo (esp. for 'Personalizado' from config)
                bool found = false;
                for (const auto &th : m_themes)
                {
                    if (th.name == theme.name)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found && !theme.name.empty())
                {
                    m_themes.push_back(theme);
                    if (m_theme_combo)
                    {
                        m_theme_combo->add_item(theme.name, theme.name);
                    }
                }

                if (m_theme_combo)
                {
                    m_theme_combo->set_selected_item_by_id(theme.name);
                }
                sync_selectors_from_theme();
            }

            void set_on_change(std::function<void()> on_change)
            {
                m_on_change = on_change;
            }

            const TerminalColorScheme &get_current_theme() const
            {
                return m_current_theme;
            }

            nlohmann::json to_json() const override
            {
                nlohmann::json j = nlohmann::json::object();

                j["use_system_theme"] = m_chk_sys_theme->is_checked();
                j["transparency"] = m_transparency;

                return j;
            }

        private:
            Checkbox<AquaObject> *m_chk_sys_theme;
            Label *m_transparency_label;
            Slider *m_transparency_slider;
            int m_transparency;
            std::function<void()> m_on_change;
            Combo *m_theme_combo;
            std::vector<TerminalColorScheme> m_themes;
            TerminalColorScheme m_current_theme;

            ColorSelector *m_bg_selector;
            ColorSelector *m_fg_selector;
            ColorSelector *m_cursor_selector;

            std::vector<ColorSelector *> m_normal_selectors;
            std::vector<ColorSelector *> m_bright_selectors;

            void sync_selectors_from_theme()
            {
                if (m_bg_selector)
                    m_bg_selector->set_color(Color(m_current_theme.primary.background));
                if (m_fg_selector)
                    m_fg_selector->set_color(Color(m_current_theme.primary.foreground));
                if (m_cursor_selector)
                    m_cursor_selector->set_color(Color(m_current_theme.primary.cursor));

                auto normal_colors = m_current_theme.normal.to_vector();
                auto bright_colors = m_current_theme.bright.to_vector();

                for (size_t i = 0; i < 8 && i < m_normal_selectors.size(); ++i)
                {
                    m_normal_selectors[i]->set_color(Color(normal_colors[i]));
                }

                for (size_t i = 0; i < 8 && i < m_bright_selectors.size(); ++i)
                {
                    m_bright_selectors[i]->set_color(Color(bright_colors[i]));
                }
            }

            void ensure_custom_theme()
            {
                if (m_current_theme.name == "Personalizado")
                    return;

                m_current_theme.name = "Personalizado";

                // Check if it's already in the list to update logic
                bool found = false;
                for (auto &th : m_themes)
                {
                    if (th.name == "Personalizado")
                    {
                        th = m_current_theme;
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    m_themes.push_back(m_current_theme);
                    if (m_theme_combo)
                    {
                        m_theme_combo->add_item("Personalizado", "Personalizado");
                    }
                }

                if (m_theme_combo)
                {
                    m_theme_combo->set_selected_item_by_id("Personalizado");
                }
            }
        };

    } // namespace terminal
} // namespace horizon
