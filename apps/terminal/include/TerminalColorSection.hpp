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
                m_theme_combo->set_fixed_size(30);
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
                lbl_opacity->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);

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

                general_page->add_child(std::move(combo_theme));
                general_page->add_child(std::move(chk_sys_theme));
                general_page->add_child(std::move(lbl_opacity));
                general_page->add_child(std::move(row_slider));

                nb->add_tab(NotebookPage("General", std::move(general_page)));
                nb->add_tab(NotebookPage("Paleta", std::make_unique<Widget>()));

                add_child(std::move(nb));
            }

            void from_json(const nlohmann::json &j) override
            {
                if (j.is_null())
                    return;

                m_original_json = j;
                m_transparency = j.value("transparency", 100);
                m_transparency_label->set_text(std::to_string(m_transparency) + "%");

                if (j.contains("use_system_theme"))
                    m_chk_sys_theme->set_checked(j["use_system_theme"].get<bool>());

                if (m_transparency_slider)
                    m_transparency_slider->set_value(static_cast<float>(m_transparency));
            }

            void set_current_theme(const TerminalColorScheme& theme) {
                m_current_theme = theme;
                if (m_theme_combo) {
                    m_theme_combo->set_selected_item_by_id(theme.name);
                }
            }

            void set_on_change(std::function<void()> on_change) {
                m_on_change = on_change;
            }

            const TerminalColorScheme& get_current_theme() const {
                return m_current_theme;
            }

            nlohmann::json to_json() const override
            {
                nlohmann::json j = m_original_json;

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
            nlohmann::json m_original_json;
            Combo *m_theme_combo;
            std::vector<TerminalColorScheme> m_themes;
            TerminalColorScheme m_current_theme;
        };

    } // namespace terminal
} // namespace horizon
