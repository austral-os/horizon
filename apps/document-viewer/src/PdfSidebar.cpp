#include "PdfSidebar.hpp"
#include "horizon/pdf/PdfThumbnailWidget.hpp"
#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Logger.hpp>
#include <cairo.h>

namespace horizon {
namespace pdf {

PdfSidebar::PdfSidebar() : ScrollArea() {
    set_fixed_size(180); // Ensure VPanel doesn't collapse the sidebar
    set_layout_type(WIDGET_LAYOUT_VERTICAL);
}

void PdfSidebar::set_document(std::shared_ptr<PdfDocument> doc) {
    m_document = doc;
    m_thumbnails.clear();
    
    if (!m_document) return;
    
    // Crear un contenedor para la lista de miniaturas
    auto container = std::make_unique<horizon::Widget>();
    container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    container->set_width(160);
    container->set_spacing(15);
    container->set_margin(10);
    
    int page_count = m_document->page_count();
    int thumb_h = 200;
    int spacing = 15;
    int margin = 10;
    
    // Calcular altura total del contenedor para habilitar scroll
    int total_height = (margin * 2) + (page_count * thumb_h) + (std::max(0, page_count - 1) * spacing);
    container->set_height(total_height);
    container->set_fixed_size(total_height);
    
    for (int i = 0; i < page_count; ++i) {
        auto thumb = std::make_unique<PdfThumbnailWidget>(i);
        thumb->set_document(m_document);
        
        // Guardar referencia para gestión de selección
        auto* thumb_ptr = thumb.get();
        m_thumbnails.push_back(thumb_ptr);
        
        // Conectar señal de selección
        thumb->when_page_selected.connect([this, thumb_ptr, i](int index) {
            // Deseleccionar todos
            for (auto* t : m_thumbnails) {
                t->set_selected(false);
            }
            // Seleccionar el actual
            thumb_ptr->set_selected(true);
            
            // Notificar hacia arriba para cambiar página en el visor
            this->when_page_selected.run(index);
        });
        
        container->add_child(std::move(thumb));
    }
    
    // Seleccionar por defecto la primera página
    if (!m_thumbnails.empty()) {
        m_thumbnails[0]->set_selected(true);
    }
    
    // El ScrollArea ahora contiene directamente esta lista
    set_content(std::move(container));
    
    // Forzar actualización de layout interno
    invalidate();
}

void PdfSidebar::update_selection_from_scroll(int page_index) {
    for (size_t i = 0; i < m_thumbnails.size(); ++i) {
        m_thumbnails[i]->set_selected((int)i == page_index);
    }
}

void PdfSidebar::render(horizon::GraphicsContext &ctx, int cx, int cy, int cw, int ch, bool force) {
    if (!is_visible()) return;

    // Dibujar el fondo ANTES que cualquier otra cosa
    auto* cr = static_cast<cairo_t*>(ctx.getNativeContext());
    if (cr) {
        int bx = x();
        int by = y();
        int bw = width();
        int bh = height();

        auto* tm = theme_manager();
        Color bg = tm ? tm->get_color("window_bg") : Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color border = tm ? tm->get_color("window_border") : Color(0.85f, 0.85f, 0.85f, 1.0f);

        cairo_set_source_rgba(cr, bg.r, bg.g, bg.b, bg.a);
        cairo_rectangle(cr, bx, by, bw, bh);
        cairo_fill(cr);

        // Línea separadora a la derecha
        cairo_set_source_rgba(cr, border.r, border.g, border.b, border.a);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, bx + bw - 1.0, by);
        cairo_line_to(cr, bx + bw - 1.0, by + bh);
        cairo_stroke(cr);
    }

    // Llamar a ScrollArea::render para que gestione el clipping y renderice los hijos
    ScrollArea::render(ctx, cx, cy, cw, ch, force);
}

void PdfSidebar::draw(horizon::GraphicsContext &ctx) {
    ScrollArea::draw(ctx);
}

} // namespace pdf
} // namespace horizon
