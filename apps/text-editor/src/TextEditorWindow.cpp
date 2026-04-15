#include "TextEditorWindow.hpp"
#include "TextEditorToolbar.hpp"
#include <horizon/text/TextEditorWidget.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/dialogs/FileDialog.hpp>

namespace horizon {
namespace text_editor {

TextEditorWindow::TextEditorWindow() 
    : ApplicationWindow(i18n().tr("text_editor.title")) {
    set_size(800, 600);
    setup_ui();
}

void TextEditorWindow::setup_ui() {
    // 1. Toolbar
    auto text_toolbar = std::make_unique<TextEditorToolbar>();
    auto* tb_ptr = text_toolbar.get();
    toolbar()->add_toolbar_widget(std::move(text_toolbar));
    
    tb_ptr->when_new_clicked.connect([this](EventContext&) { this->new_file(); });
    tb_ptr->when_open_clicked.connect([this](EventContext&) {
        auto dialog = std::make_unique<FileDialog>(FileDialogMode::Open);
        dialog->when_accepted.connect([this](FileDialogAcceptedContext& ctx) {
            this->open_file(ctx.selected_path);
        });
        
        std::thread([d = std::move(dialog)]() mutable {
            d->run();
        }).detach();
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

    // 2. Tab Collection
    auto tabs = std::make_unique<TabCollection>();
    m_tabs = tabs.get();
    m_tabs->set_smart_header(false);
    m_tabs->set_closable_tabs(true);
    
    m_tabs->when_tab_close_requested.connect([this](int index) {
        m_tabs->remove_tab(index);
        if (m_tabs->tab_count() == 0) {
            new_file();
        }
    });
    
    m_tabs->when_add_tab_clicked.connect([this](EventContext&) {
        new_file();
    });

    set_content(std::move(tabs));
}

void TextEditorWindow::create_tab(const std::string& title, std::shared_ptr<horizon::text::TextDocument> doc) {
    auto scroll = std::make_unique<ScrollArea>();
    auto editor = std::make_unique<horizon::text::TextEditorWidget>();
    auto* editor_ptr = editor.get();
    editor->set_document(doc);
    
    scroll->set_content(std::move(editor));
    m_tabs->add_tab(title, std::move(scroll));
    m_tabs->set_current_tab(m_tabs->tab_count() - 1);
    
    if (application()) {
        application()->set_focused_widget(editor_ptr);
    }
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
        m_documents.push_back(doc);
        create_tab(path, doc);
    }
}

void TextEditorWindow::save_current_file() {
    // Logic for saving...
}

} // namespace text_editor
} // namespace horizon
