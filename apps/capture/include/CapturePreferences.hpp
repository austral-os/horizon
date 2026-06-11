#pragma once

#include <functional>
#include <horizon/Checkbox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/FileSelector.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon::capture
{

    // --- General Section ---
    class CaptureGeneralSection : public Widget, public ConfigSection
    {
    public:
        CaptureGeneralSection(std::function<void()> on_change) : m_on_change(on_change)
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            set_margin(30);
            set_spacing(15);

            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            // row->set_fixed_size(35);
            row->set_spacing(15);

            auto label =
                std::make_unique<Label>(horizon::i18n().tr("capture.preferences.output_directory"));
            label->set_fixed_size(35);
            row->add_child(std::move(label));

            auto file_selector = std::make_unique<FileSelector>(FileDialogMode::SelectFolder);
            file_selector->set_fixed_size(35);
            m_file_selector = file_selector.get();
            m_file_selector->when_path_changed.connect(
                [this](const FileDialogAcceptedContext &)
                {
                    if (m_on_change)
                        m_on_change();
                });
            row->add_child(std::move(file_selector));

            add_child(std::move(row));
            add_child(Spacer());
        }

        void from_json(const nlohmann::json &j) override
        {
            if (j.is_null())
                return;
            if (j.contains("output_directory"))
            {
                m_file_selector->set_path(j["output_directory"].get<std::string>());
            }
        }

        nlohmann::json to_json() const override
        {
            nlohmann::json j = nlohmann::json::object();
            j["output_directory"] = m_file_selector->path();
            return j;
        }

    private:
        FileSelector *m_file_selector;
        std::function<void()> m_on_change;
    };

    // --- Image Section ---
    class CaptureImageSection : public Widget, public ConfigSection
    {
    public:
        CaptureImageSection(std::function<void()> on_change) : m_on_change(on_change)
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            set_margin(30);
            set_spacing(15);

            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(35);
            row->set_spacing(15);

            auto label =
                std::make_unique<Label>(horizon::i18n().tr("capture.preferences.image_format"));
            label->set_fixed_size(150);
            row->add_child(std::move(label));

            auto combo = std::make_unique<Combo>();
            combo->add_item("png", horizon::i18n().tr("capture.preferences.format_png"));
            combo->add_item("jpg", horizon::i18n().tr("capture.preferences.format_jpg"));
            combo->add_item("bmp", horizon::i18n().tr("capture.preferences.format_bmp"));
            combo->set_fixed_size(250);
            m_format_combo = combo.get();
            m_format_combo->when_item_selected.connect(
                [this](const ComboItemSelectedContext &)
                {
                    if (m_on_change)
                        m_on_change();
                });
            row->add_child(std::move(combo));
            row->add_child(Spacer());

            add_child(std::move(row));
            add_child(Spacer());
        }

        void from_json(const nlohmann::json &j) override
        {
            if (j.is_null())
                return;
            if (j.contains("format"))
            {
                m_format_combo->set_selected_item_by_id(j["format"].get<std::string>());
            }
        }

        nlohmann::json to_json() const override
        {
            nlohmann::json j = nlohmann::json::object();
            if (auto selected = m_format_combo->selected_item())
            {
                j["format"] = selected->id;
            }
            else
            {
                j["format"] = "png";
            }
            return j;
        }

    private:
        Combo *m_format_combo;
        std::function<void()> m_on_change;
    };

    // --- Video Section ---
    class CaptureVideoSection : public Widget, public ConfigSection
    {
    public:
        CaptureVideoSection(std::function<void()> on_change) : m_on_change(on_change)
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            set_margin(30);
            set_spacing(15);

            // Video Format Row
            auto row1 = std::make_unique<Widget>();
            row1->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row1->set_fixed_size(35);
            row1->set_spacing(15);

            auto label_fmt =
                std::make_unique<Label>(horizon::i18n().tr("capture.preferences.video_container"));
            label_fmt->set_fixed_size(150);
            row1->add_child(std::move(label_fmt));

            auto combo_fmt = std::make_unique<Combo>();
            combo_fmt->add_item("mp4", horizon::i18n().tr("capture.preferences.format_mp4"));
            combo_fmt->add_item("webm", horizon::i18n().tr("capture.preferences.format_webm"));
            combo_fmt->set_fixed_size(250);
            m_format_combo = combo_fmt.get();
            m_format_combo->when_item_selected.connect(
                [this](const ComboItemSelectedContext &)
                {
                    if (m_on_change)
                        m_on_change();
                });
            row1->add_child(std::move(combo_fmt));
            row1->add_child(Spacer());
            add_child(std::move(row1));

            // Quality Row
            auto row2 = std::make_unique<Widget>();
            row2->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row2->set_fixed_size(35);
            row2->set_spacing(15);

            auto label_qual = std::make_unique<Label>(
                horizon::i18n().tr("capture.preferences.recording_quality"));
            label_qual->set_fixed_size(150);
            row2->add_child(std::move(label_qual));

            auto combo_qual = std::make_unique<Combo>();
            combo_qual->add_item("low", horizon::i18n().tr("capture.preferences.quality_low"));
            combo_qual->add_item("medium",
                                 horizon::i18n().tr("capture.preferences.quality_medium"));
            combo_qual->add_item("high", horizon::i18n().tr("capture.preferences.quality_high"));
            combo_qual->set_fixed_size(250);
            m_quality_combo = combo_qual.get();
            m_quality_combo->when_item_selected.connect(
                [this](const ComboItemSelectedContext &)
                {
                    if (m_on_change)
                        m_on_change();
                });
            row2->add_child(std::move(combo_qual));
            row2->add_child(Spacer());
            add_child(std::move(row2));

            add_child(Spacer());
        }

        void from_json(const nlohmann::json &j) override
        {
            if (j.is_null())
                return;
            if (j.contains("container"))
            {
                m_format_combo->set_selected_item_by_id(j["container"].get<std::string>());
            }
            if (j.contains("quality"))
            {
                m_quality_combo->set_selected_item_by_id(j["quality"].get<std::string>());
            }
        }

        nlohmann::json to_json() const override
        {
            nlohmann::json j = nlohmann::json::object();
            if (auto selected = m_format_combo->selected_item())
            {
                j["container"] = selected->id;
            }
            else
            {
                j["container"] = "mp4";
            }
            if (auto selected = m_quality_combo->selected_item())
            {
                j["quality"] = selected->id;
            }
            else
            {
                j["quality"] = "medium";
            }
            return j;
        }

    private:
        Combo *m_format_combo;
        Combo *m_quality_combo;
        std::function<void()> m_on_change;
    };

} // namespace horizon::capture
