#include "DocumentWindow.hpp"
#include "DocumentToolbar.hpp"
#include "PdfSidebar.hpp"
#include "horizon/pdf/PdfWidget.hpp"
#include "horizon/pdf/PdfThumbnailWidget.hpp"
#include <horizon/VPanel.hpp>
#include <horizon/TabCollection.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/I18n.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/dialogs/FileDialog.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Application.hpp>

namespace horizon {
namespace pdf {

DocumentWindow::DocumentWindow() 
    : ApplicationWindow("Document Viewer") {
    
    std::string title = i18n().tr("app.title");
    if (title != "app.title") {
        set_title(title);
    }
    
    build_content();
    build_toolbar();
}

DocumentWindow::~DocumentWindow() {}

void DocumentWindow::build_toolbar() {
    auto tb = toolbar();
    if (!tb) return;

    auto dtb = std::make_unique<DocumentToolbar>();
    
    dtb->when_open_clicked.connect([this](const horizon::EventContext&) {
        auto dialog = std::make_unique<horizon::FileDialog>(horizon::FileDialogMode::Open, i18n().tr("dialog.open_pdf"));
        dialog->when_accepted.connect([this](const horizon::FileDialogAcceptedContext& ev) {
            this->open_file(ev.selected_path);
        });
        dialog->run();
    });

    dtb->when_zoom_clicked.connect([this](const horizon::GroupButtonClickEvent& ev) {
        if (ev.button_index == 0) on_zoom_out();
        else if (ev.button_index == 1) on_zoom_in();
        else if (ev.button_index == 2) on_zoom_fit();
    });

    dtb->when_view_clicked.connect([this](const horizon::GroupButtonClickEvent& ev) {
        if (ev.button_index == 0) on_toggle_fullscreen();
        else if (ev.button_index == 1) {
            if (application()) application()->show_preferences();
        }
    });

    tb->add_toolbar_widget(std::move(dtb));
}

void DocumentWindow::build_content() {
    auto tabs = std::make_unique<horizon::TabCollection>();
    m_tabs = tabs.get();
    
    // Configuración estilo Nova
    m_tabs->set_smart_header(true);
    m_tabs->set_closable_tabs(true);
    m_tabs->set_position_type(horizon::FILL);

    // Botón "+" para abrir nuevos archivos
    m_tabs->when_add_tab_clicked.connect([this](horizon::EventContext&) {
        auto dialog = std::make_unique<horizon::FileDialog>(horizon::FileDialogMode::Open, i18n().tr("dialog.open_pdf"));
        dialog->when_accepted.connect([this](const horizon::FileDialogAcceptedContext& ev) {
            this->open_file(ev.selected_path);
        });
        dialog->run();
    });

    // Cierre de pestañas
    m_tabs->when_tab_close_requested.connect([this](int index) {
        if (application()) {
            application()->post_task([this, index]() {
                m_tabs->remove_tab(index);
                if (m_tabs->tab_count() == 0) {
                    // Opcional: Cerrar ventana si no hay archivos
                }
            });
        }
    });

    set_content(std::move(tabs));
}

void DocumentWindow::open_file(const std::string& path) {
    auto doc = PdfDocument::open(path);
    if (!doc) return;

    LOG_INFO << "DocumentViewer: Opening " << path;

    auto shared_doc = std::shared_ptr<PdfDocument>(doc.release());
    
    // Layout principal de la pestaña: Panel con divisor (Sidebar | Contenido)
    auto vpanel = std::make_unique<horizon::VPanel>();
    vpanel->set_left_width(180);
    
    // 1. Sidebar de miniaturas
    auto sidebar = std::make_unique<PdfSidebar>();
    auto* sidebar_ptr = sidebar.get();
    sidebar_ptr->set_document(shared_doc);
    
    // 2. Visor principal (ScrollArea + PdfWidget)
    auto scroll = std::make_unique<horizon::ScrollArea>();
    auto* scroll_ptr = scroll.get();
    scroll_ptr->set_position_type(horizon::FILL);
    
    auto widget = std::make_unique<PdfWidget>();
    auto* pdf_ptr = widget.get();
    widget->set_document(shared_doc);
    scroll->set_content(std::move(widget));
    
    // Conectar navegación: click en miniatura -> scroll a la página
    sidebar_ptr->when_page_selected.connect([scroll_ptr, pdf_ptr](int index) {
        int y = pdf_ptr->get_page_y(index);
        scroll_ptr->set_scroll_position(0, y);
    });
    
    vpanel->add_child(std::move(sidebar));
    vpanel->add_child(std::move(scroll));
    
    std::string title = shared_doc->get_title();
    if (title.empty()) {
        size_t last_slash = path.find_last_of('/');
        title = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    }
    
    m_tabs->add_tab(title, std::move(vpanel));
    m_tabs->set_current_tab(m_tabs->tab_count() - 1);
}

void DocumentWindow::on_zoom_in() {
}

void DocumentWindow::on_zoom_out() {
}

void DocumentWindow::on_zoom_fit() {
}

void DocumentWindow::on_toggle_fullscreen() {
    set_immersive_mode(!m_is_immersive);
}

} // namespace pdf
} // namespace horizon
