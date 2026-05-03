#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/Label.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/FileSelector.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>
#include <functional>
#include <string>

namespace horizon::capture {

// --- General Section ---
class CaptureGeneralSection : public Widget, public ConfigSection {
public:
    CaptureGeneralSection(std::function<void()> on_change) : m_on_change(on_change) {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(15);

        auto label = std::make_unique<Label>("Output Directory");
        label->set_font_weight(FONT_WEIGHT_BOLD);
        add_child(std::move(label));

        auto file_selector = std::make_unique<FileSelector>(FileDialogMode::SelectFolder);
        m_file_selector = file_selector.get();
        m_file_selector->when_path_changed.connect([this](const FileDialogAcceptedContext &) {
            if (m_on_change) m_on_change();
        });
        add_child(std::move(file_selector));
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        if (j.contains("output_directory")) {
            m_file_selector->set_path(j["output_directory"].get<std::string>());
        }
    }

    nlohmann::json to_json() const override {
        nlohmann::json j = nlohmann::json::object();
        j["output_directory"] = m_file_selector->path();
        return j;
    }

private:
    FileSelector *m_file_selector;
    std::function<void()> m_on_change;
};

// --- Image Section ---
class CaptureImageSection : public Widget, public ConfigSection {
public:
    CaptureImageSection(std::function<void()> on_change) : m_on_change(on_change) {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(15);

        auto label = std::make_unique<Label>("Image Format");
        label->set_font_weight(FONT_WEIGHT_BOLD);
        add_child(std::move(label));

        auto combo = std::make_unique<Combo>();
        combo->add_item("png", "PNG (Lossless)");
        combo->add_item("jpg", "JPEG (Compressed)");
        combo->add_item("bmp", "Bitmap (Raw)");
        combo->set_width(200);
        m_format_combo = combo.get();
        m_format_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) {
            if (m_on_change) m_on_change();
        });
        add_child(std::move(combo));
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        if (j.contains("format")) {
            m_format_combo->set_selected_item_by_id(j["format"].get<std::string>());
        }
    }

    nlohmann::json to_json() const override {
        nlohmann::json j = nlohmann::json::object();
        if (auto selected = m_format_combo->selected_item()) {
            j["format"] = selected->id;
        } else {
            j["format"] = "png";
        }
        return j;
    }

private:
    Combo *m_format_combo;
    std::function<void()> m_on_change;
};

// --- Video Section ---
class CaptureVideoSection : public Widget, public ConfigSection {
public:
    CaptureVideoSection(std::function<void()> on_change) : m_on_change(on_change) {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(15);

        // Video Format
        auto label_fmt = std::make_unique<Label>("Video Container");
        label_fmt->set_font_weight(FONT_WEIGHT_BOLD);
        add_child(std::move(label_fmt));

        auto combo_fmt = std::make_unique<Combo>();
        combo_fmt->add_item("mp4", "MP4 (H.264 / AAC)");
        combo_fmt->add_item("webm", "WebM (VP9 / Opus)");
        combo_fmt->set_width(200);
        m_format_combo = combo_fmt.get();
        m_format_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) {
            if (m_on_change) m_on_change();
        });
        add_child(std::move(combo_fmt));

        // Quality
        auto label_qual = std::make_unique<Label>("Recording Quality");
        label_qual->set_font_weight(FONT_WEIGHT_BOLD);
        add_child(std::move(label_qual));

        auto combo_qual = std::make_unique<Combo>();
        combo_qual->add_item("low", "Low (Small size)");
        combo_qual->add_item("medium", "Medium (Balanced)");
        combo_qual->add_item("high", "High (High Bitrate)");
        combo_qual->set_width(200);
        m_quality_combo = combo_qual.get();
        m_quality_combo->when_item_selected.connect([this](const ComboItemSelectedContext &) {
            if (m_on_change) m_on_change();
        });
        add_child(std::move(combo_qual));
    }

    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        if (j.contains("container")) {
            m_format_combo->set_selected_item_by_id(j["container"].get<std::string>());
        }
        if (j.contains("quality")) {
            m_quality_combo->set_selected_item_by_id(j["quality"].get<std::string>());
        }
    }

    nlohmann::json to_json() const override {
        nlohmann::json j = nlohmann::json::object();
        if (auto selected = m_format_combo->selected_item()) {
            j["container"] = selected->id;
        } else {
            j["container"] = "mp4";
        }
        if (auto selected = m_quality_combo->selected_item()) {
            j["quality"] = selected->id;
        } else {
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
