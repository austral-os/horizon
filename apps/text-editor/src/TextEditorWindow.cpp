#include "TextEditorWindow.hpp"
#include "TextEditorToolbar.hpp"
#include <horizon/text/TextEditorWidget.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/dialogs/FileDialog.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

namespace horizon {
namespace text_editor {

namespace {
std::vector<FileFilter> text_file_filters() {
    return {
        {"Archivo TXT", {"*.txt"}, FileFilterUsage::All},
        {"Archivo MD", {"*.md"}, FileFilterUsage::All},
        {"All Files", {"*"}, FileFilterUsage::All}
    };
}
}

    TextEditorWindow::TextEditorWindow() 
        : ApplicationWindow("Text Editor") {
        // Load language preference from config BEFORE setting title or building UI,
        // so the correct locale is active for all i18n calls.
        std::string lang = load_language_setting();
        if (lang.empty() || lang == "default") {
            // Follow global/fallback locale chain
            i18n().load_app_locales("text-editor");
        } else {
            // Load the specific app locale
            if (!i18n().load_app_locale("text-editor", lang)) {
                // Invalid or missing locale — fall back to global default
                LOG_WARNING << "TextEditor: language '" << lang << "' not found, falling back to global locale";
                i18n().load_app_locales("text-editor");
            }
        }

        set_title(i18n().tr("text_editor.title"));
        set_size(1024, 768);
        setup_ui();

        when_file_opened.connect([this](Window::FileOpenedContext& ctx) {
            if (!ctx.paths.empty()) {
                for (const auto& path : ctx.paths) {
                    if (!path.empty()) {
                        this->open_file(path);
                    }
                }
                return;
            }

            if (!ctx.path.empty()) {
                this->open_file(ctx.path);
            }
        });

        set_accept_drops(true);
        when_drop.connect([this](DropEventContext &ctx) {
            auto data = ctx.get_data("text/uri-list");
            if (!data.empty()) {
                std::string content(data.begin(), data.end());
                if (content.find("file://") == 0) {
                    size_t end = content.find("\r\n");
                    std::string path = content.substr(7, (end == std::string::npos) ? std::string::npos : end - 7);
                    this->open_file(path);
                }
            }
        });

        when_file_close.connect([this](EventContext&) {
            if (m_tabs && m_tabs->tab_count() > 0) {
                m_tabs->remove_tab(m_tabs->current_tab_index());
                if (m_tabs->tab_count() == 0) {
                    new_file();
                }
            }
        });

        when_save.connect([this](Window::FileSaveContext& ctx) {
            auto* scroll = dynamic_cast<ScrollArea*>(m_tabs->current_tab_body());
            if (scroll && !scroll->children().empty()) {
                auto* editor = dynamic_cast<horizon::text::TextEditorWidget*>(scroll->children()[0].get());
                if (editor && editor->get_document()) {
                    if (editor->get_document()->save_to_file(ctx.path)) {
                        editor->get_document()->clear_dirty();
                        LOG_INFO << "File saved successfully to: " << ctx.path;
                    }
                }
            }
        });

        when_save_as.connect([this](Window::FileSaveContext& ctx) {
            auto* scroll = dynamic_cast<ScrollArea*>(m_tabs->current_tab_body());
            if (scroll && !scroll->children().empty()) {
                auto* editor = dynamic_cast<horizon::text::TextEditorWidget*>(scroll->children()[0].get());
                if (editor && editor->get_document()) {
                    if (editor->get_document()->save_to_file(ctx.path)) {
                        editor->get_document()->set_path(ctx.path);
                        editor->get_document()->clear_dirty();
                        m_tabs->set_tab_title(m_tabs->current_tab_index(), ctx.path);
                        LOG_INFO << "File saved as: " << ctx.path;
                    }
                }
            }
        });

        load_settings();
    }

void TextEditorWindow::setup_ui() {
    // 1. Toolbar
    auto text_toolbar = std::make_unique<TextEditorToolbar>();
    auto* tb_ptr = text_toolbar.get();
    toolbar()->add_toolbar_widget(std::move(text_toolbar));
    
    tb_ptr->when_new_clicked.connect([this](EventContext&) { this->new_file(); });
    tb_ptr->when_open_clicked.connect([this](EventContext&) {
        SignalContext ctx;
        application()->signal_manager.emit("file.open", ctx);
    });

    tb_ptr->when_save_clicked.connect([this](EventContext&) {
        SignalContext ctx;
        application()->signal_manager.emit("file.save", ctx);
    });
    
    tb_ptr->when_undo_clicked.connect([this](EventContext&) {
        auto* scroll = dynamic_cast<ScrollArea*>(m_tabs->current_tab_body());
        if (scroll) {
            auto* editor = dynamic_cast<horizon::text::TextEditorWidget*>(scroll->children()[0].get());
            if (editor && editor->get_document()) editor->get_document()->undo();
        }
    });

    tb_ptr->when_redo_clicked.connect([this](EventContext&) {
        auto* scroll = dynamic_cast<ScrollArea*>(m_tabs->current_tab_body());
        if (scroll) {
            auto* editor = dynamic_cast<horizon::text::TextEditorWidget*>(scroll->children()[0].get());
            if (editor && editor->get_document()) editor->get_document()->redo();
        }
    });

    tb_ptr->when_preferences_clicked.connect([this](EventContext&) {
        if (application()) application()->show_preferences();
    });

    tb_ptr->when_fullscreen_clicked.connect([this](EventContext&) {
        if (application()) {
            if (application()->is_fullscreen()) application()->unfullscreen();
            else application()->fullscreen();
        }
    });

    // 2. Tab Collection
    auto tabs = std::make_unique<TabCollection>();
    m_tabs = tabs.get();
    m_tabs->set_smart_header(true);
    m_tabs->set_closable_tabs(true);
    
        m_tabs->when_tab_close_requested.connect([this](int index) {
            // Save-check: ask confirmation if the tab's content has unsaved changes
            Widget *body = m_tabs->tab_body(index);
            if (body && application() && application()->has_dirty_save_check_widgets(body)) {
                bool should_close = application()->confirm(
                    i18n().tr("text_editor.save_check.tab_unsaved_message"),
                    i18n().tr("text_editor.save_check.tab_unsaved_title"));
                if (!should_close) return;
            }
            m_tabs->remove_tab(index);
            if (m_tabs->tab_count() == 0) {
                new_file();
            }
        });
    
    m_tabs->when_add_tab_clicked.connect([this](EventContext&) {
        new_file();
    });

    m_tabs->when_tab_selected.connect([this](int) {
        this->update_status_bar();
    });

    // 3. Status Bar
    show_status_bar();
    auto status_lbl = std::make_unique<horizon::Label>("Ln 1, Col 1 | Total: 1");
    m_status_label = status_lbl.get();
    statusbar()->add_child(std::move(status_lbl));

    set_content(std::move(tabs));
}

void TextEditorWindow::create_tab(const std::string& title, std::shared_ptr<horizon::text::TextDocument> doc) {
    auto scroll = std::make_unique<ScrollArea>();
    auto editor = std::make_unique<horizon::text::TextEditorWidget>();
    auto* editor_ptr = editor.get();
    editor->set_document(doc);
    editor->set_position_type(WidgetPositionTypes::FREE);
    editor->set_highlight_current_line(true);
    
    scroll->set_content(std::move(editor));
    m_tabs->add_tab(title, std::move(scroll));
    m_tabs->set_current_tab(m_tabs->tab_count() - 1);
    
    if (application()) {
        application()->set_focused_widget(editor_ptr);
    }

    // Apply current settings
    load_settings();

    editor_ptr->when_cursor_moved.connect([this](EventContext&) { this->update_status_bar(); });
    doc->on_changed = [this, editor_ptr]() {
        // Do NOT call calculate_layout() here: it calls pango_layout_get_pixel_size()
        // which is O(n_lines) and fires on every keystroke (including cursor-only moves).
        // The widget handles layout rebuilding lazily via the version check in calculate_layout().
        editor_ptr->invalidate();
        this->update_status_bar();
    };

    update_status_bar();
}

void TextEditorWindow::new_file() {
    auto doc = std::make_shared<horizon::text::TextDocument>();
    doc->set_text("");
    m_documents.push_back(doc);
    create_tab("Untitled", doc);
}

void TextEditorWindow::open_file(const std::string& path) {
    auto doc = std::make_shared<horizon::text::TextDocument>();
    if (doc->load_from_file(path)) {
        doc->set_path(path); // Set path after successful load
        m_documents.push_back(doc);
        create_tab(path, doc);
    }
}

std::string TextEditorWindow::current_file_path() const {
    if (!m_tabs) return "";
    auto* scroll = dynamic_cast<ScrollArea*>(m_tabs->current_tab_body());
    if (scroll && !scroll->children().empty()) {
        auto* editor = dynamic_cast<horizon::text::TextEditorWidget*>(scroll->children()[0].get());
        if (editor && editor->get_document()) {
            return editor->get_document()->get_path();
        }
    }
    return "";
}

std::vector<FileFilter> TextEditorWindow::file_filters() const {
    return text_file_filters();
}

void TextEditorWindow::save_current_file() {
    // Logic for saving...
}

void TextEditorWindow::update_status_bar() {
    if (!m_status_label || !m_tabs) return;

    auto* scroll = dynamic_cast<ScrollArea*>(m_tabs->current_tab_body());
    if (!scroll) return;

    if (scroll->children().empty()) return;
    auto* editor = dynamic_cast<horizon::text::TextEditorWidget*>(scroll->children()[0].get());
    if (!editor || !editor->get_document()) return;

    int row, col;
    editor->get_document()->get_cursor_row_col(row, col);
    int total = editor->get_document()->get_line_count();

    std::string text = i18n().tr("text_editor.status.line") + " " + std::to_string(row) + ", " +
                       i18n().tr("text_editor.status.col") + " " + std::to_string(col) + " | " +
                       i18n().tr("text_editor.status.total") + ": " + std::to_string(total);
    
    m_status_label->set_text(text);
}

std::string TextEditorWindow::get_config_path() {
    char *home = std::getenv("HOME");
    return home ? std::string(home) + "/.config/horizon/text-editor.json" : "text-editor.json";
}

std::string TextEditorWindow::load_language_setting() {
    std::string path = get_config_path();
    std::ifstream file(path);
    if (!file.is_open()) return "default";

    try {
        nlohmann::json j;
        file >> j;
        if (j.contains("editor") && j["editor"].contains("language")) {
            return j["editor"]["language"].get<std::string>();
        }
    } catch (...) {
        // Config parse error — fall back to default
    }
    return "default";
}

void TextEditorWindow::load_settings() {
    std::string path = get_config_path();
    std::ifstream file(path);
    if (!file.is_open()) return;

    try {
        nlohmann::json j;
        file >> j;
        if (j.contains("editor")) {
            auto config = j["editor"];

            // Apply color scheme variant before other settings
            std::string variant = config.value("variant", "default");
            if (!ThemeManager::instance().set_app_color_scheme_variant("text-editor", variant)) {
                ThemeManager::instance().set_app_color_scheme_variant("text-editor", "default");
            }

            for (int i = 0; i < m_tabs->tab_count(); ++i) {
                auto* scroll = dynamic_cast<ScrollArea*>(m_tabs->tab_body(i));
                if (scroll && !scroll->children().empty()) {
                    auto* editor = dynamic_cast<horizon::text::TextEditorWidget*>(scroll->children()[0].get());
                    if (editor) {
                        if (config.contains("font")) editor->set_font_family(config["font"]);
                        if (config.contains("font_size")) editor->set_font_size(config["font_size"]);
                        if (config.contains("font_weight")) editor->set_font_weight(config["font_weight"]);
                        if (config.contains("highlight_line")) editor->set_highlight_current_line(config["highlight_line"]);
                        if (config.contains("show_line_numbers")) editor->set_show_line_numbers(config["show_line_numbers"]);
                    }
                }
            }
        }
    } catch (...) {
        // Log error
    }
}

} // namespace text_editor
} // namespace horizon
