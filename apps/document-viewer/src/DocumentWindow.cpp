#include "DocumentWindow.hpp"
#include "DocumentToolbar.hpp"
#include "PdfSidebar.hpp"
#include "PdfTabContent.hpp"
#include "horizon/pdf/PdfWidget.hpp"
#include "horizon/pdf/PdfThumbnailWidget.hpp"
#include <horizon/VPanel.hpp>
#include <horizon/TabCollection.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/I18n.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Menu.hpp>
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
    
    // Conectar eventos estándar de archivos
    when_file_opened.connect([this](Window::FileOpenedContext& ctx) {
        this->open_file(ctx.path);
    });

    when_file_close.connect([this](EventContext&) {
        if (m_tabs && m_tabs->tab_count() > 0) {
            m_tabs->remove_tab(m_tabs->current_tab_index());
        }
    });

    // Statusbar setup
    // Statusbar setup (Estilo Nova/Arkfm)
    show_status_bar();
    auto sb = statusbar();
    if (sb) {
        sb->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        
        // 1. Margen izquierdo y Etiqueta de estado
        sb->add_child(horizon::Spacer(10));
        
        auto lbl = std::make_unique<horizon::Label>("");
        m_status_label = lbl.get();
        sb->add_child(std::move(lbl));
        
        // 2. Espaciador flexible para empujar el progreso a la derecha
        sb->add_child(horizon::Spacer());
        
        // 3. Contenedor para ProgressBar (Centrado verticalmente)
        auto pbc = std::make_unique<horizon::Widget>();
        pbc->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        pbc->set_fixed_size(200); // Ancho para el indicador
        
        pbc->add_child(horizon::Spacer());
        
        auto pb = std::make_unique<horizon::ProgressBar>();
        m_progress_bar = pb.get();
        m_progress_bar->set_fixed_size(10); // Altura de la barra
        m_progress_bar->set_visible(false);
        pbc->add_child(std::move(pb));
        
        pbc->add_child(horizon::Spacer());
        
        sb->add_child(std::move(pbc));
        
        // 4. Margen derecho
        sb->add_child(horizon::Spacer(10));
    }
}

DocumentWindow::~DocumentWindow() {}

void DocumentWindow::build_toolbar() {
    auto tb = toolbar();
    if (!tb) return;

    auto dtb = std::make_unique<DocumentToolbar>();
    
    dtb->when_open_clicked.connect([this](const horizon::EventContext&) {
        if (application()) {
            application()->signal_manager.emit("file.open");
        }
    });

    dtb->when_sidebar_toggled.connect([this](const horizon::EventContext&) {
        this->on_toggle_sidebar();
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
        if (application()) {
            application()->signal_manager.emit("file.open");
        }
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
    if (m_status_label) m_status_label->set_text(i18n().tr("status.loading"));
    if (m_progress_bar) {
        m_progress_bar->set_visible(true);
        m_progress_bar->set_indeterminate(true);
    }
    
    auto doc = PdfDocument::open(path);
    if (!doc) {
        if (m_status_label) m_status_label->set_text(i18n().tr("status.error_opening"));
        if (m_progress_bar) m_progress_bar->set_visible(false);
        return;
    }

    LOG_INFO << "DocumentViewer: Opening " << path;
    int page_count = doc->page_count();

    auto shared_doc = std::shared_ptr<PdfDocument>(doc.release());
    
    auto vpanel = std::make_unique<PdfTabContent>();
    vpanel->set_left_width(180);
    
    // 1. Sidebar de miniaturas
    auto sidebar = std::make_unique<PdfSidebar>();
    auto* sidebar_ptr = sidebar.get();
    sidebar_ptr->set_document(shared_doc);
    
    // Aplicar visibilidad actual
    sidebar_ptr->set_visible(m_sidebar_visible);
    vpanel->set_left_width(m_sidebar_visible ? 180 : 0);
    
    // 2. Visor principal (ScrollArea + PdfWidget)
    auto scroll = std::make_unique<horizon::ScrollArea>();
    auto* scroll_ptr = scroll.get();
    scroll_ptr->set_position_type(horizon::FILL);
    
    auto widget = std::make_unique<PdfWidget>();
    auto* pdf_ptr = widget.get();
    widget->set_document(shared_doc);
    
    // MENU CONTEXTUAL: Toggle Sidebar + Fullscreen + Clipboard
    widget->when_right_click.connect([this, pdf_ptr](horizon::MouseButtonEventContext& ev) {
        if (application()) {
            auto menu = std::make_unique<horizon::Menu>();
            
            // 1. Zoom/Vista (Opcional, pero centrémonos en lo pedido)
            
            // 2. Opción de Sidebar
            std::string sb_label = m_sidebar_visible ? "Ocultar barra lateral" : "Mostrar barra lateral";
            auto item_sb = std::make_unique<horizon::MenuItem>(sb_label);
            item_sb->set_icon("view-sidebar");
            item_sb->when_click.connect([this](const horizon::MouseButtonEventContext&) {
                this->on_toggle_sidebar();
            });
            menu->add_item(std::move(item_sb));

            // 3. Opción de Pantalla Completa (manual para dirigirla al DocumentWindow)
            menu->add_separator();
            std::string fs_label = application()->is_fullscreen() ? "Salir de pantalla completa" : "Pantalla completa";
            auto item_fs = std::make_unique<horizon::MenuItem>(fs_label);
            item_fs->set_shortcut("F11");
            item_fs->set_id("fullscreen_context");
            item_fs->when_click.connect([this](const horizon::MouseButtonEventContext&) {
                this->on_toggle_fullscreen();
            });
            menu->add_item(std::move(item_fs));
            
            // Al pasar pdf_ptr como dueño, Horizon inyectará automáticamente 
            // las opciones de "Copiar" (Clipboard) al final del menú.
            application()->show_context_menu(menu.release(), ev.x, ev.y, ev.serial, pdf_ptr);
        }
    });

    scroll->set_content(std::move(widget));
    
    // Conectar navegación: click en miniatura -> scroll a la página
    sidebar_ptr->when_page_selected.connect([scroll_ptr, pdf_ptr](int index) {
        int y = pdf_ptr->get_page_y(index);
        scroll_ptr->set_scroll_position(0, y);
    });
    
    // Conectar sincronización inversa: scroll -> seleccionar miniatura
    scroll_ptr->when_scroll.connect([scroll_ptr, pdf_ptr, sidebar_ptr](const horizon::EventContext&) {
        // Obtenemos qué página está en la parte superior del visor actualmente
        int current_idx = pdf_ptr->get_page_at_y(scroll_ptr->scroll_y());
        sidebar_ptr->update_selection_from_scroll(current_idx);
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
    
    // Actualizar estado final
    std::string status = i18n().tr("status.loaded_pages");
    size_t pos = status.find("%1");
    if (pos != std::string::npos) {
        status.replace(pos, 2, std::to_string(page_count));
    } else {
        status += " " + std::to_string(page_count);
    }
    
    if (m_status_label) m_status_label->set_text(status);
    if (m_progress_bar) {
        m_progress_bar->set_visible(false);
    }
}

void DocumentWindow::on_zoom_in() {
}

void DocumentWindow::on_zoom_out() {
}

void DocumentWindow::on_zoom_fit() {
}

void DocumentWindow::on_toggle_fullscreen() {
    if (application()) {
        application()->signal_manager.emit("fullscreen");
    }
}

void DocumentWindow::on_toggle_sidebar() {
    m_sidebar_visible = !m_sidebar_visible;
    
    // Aplicar a la pestaña actual si existe
    auto current = m_tabs->current_tab_body();
    if (current) {
        // VPanel es el widget de la pestaña
        auto vpanel = dynamic_cast<horizon::VPanel*>(current);
        if (vpanel) {
            vpanel->set_left_width(m_sidebar_visible ? 180 : 0);
            
            // Localizar el sidebar (primer hijo del VPanel)
            auto& children = vpanel->children();
            if (!children.empty()) {
                children[0]->set_visible(m_sidebar_visible);
            }
            vpanel->invalidate();
            vpanel->calculate_layout();
        }
    }
}

} // namespace pdf
} // namespace horizon
