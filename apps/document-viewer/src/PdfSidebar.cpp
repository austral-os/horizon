#include "PdfSidebar.hpp"
#include "horizon/pdf/PdfThumbnailWidget.hpp"
#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
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
    if (!m_document) return;
    
    // Crear un contenedor para la lista de miniaturas
    auto container = std::make_unique<horizon::Widget>();
    container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    container->set_width(160);
    container->set_spacing(15);
    container->set_margin(10);
    
    int page_count = m_document->page_count();
    for (int i = 0; i < page_count; ++i) {
        auto thumb = std::make_unique<PdfThumbnailWidget>(i);
        thumb->set_document(m_document);
        
        // Conectar señal de selección
        thumb->when_page_selected.connect([this](int index) {
            this->when_page_selected.run(index);
        });
        
        container->add_child(std::move(thumb));
    }
    
    // El ScrollArea ahora contiene directamente esta lista
    set_content(std::move(container));
    
    // Forzar actualización de layout interno
    invalidate();
}

void PdfSidebar::render(horizon::GraphicsContext &ctx, int cx, int cy, int cw, int ch, bool force) {
    if (!is_visible()) return;

    // Dibujar el fondo ANTES que cualquier otra cosa (incluso antes del clip del ScrollArea)
    auto* cr = static_cast<cairo_t*>(ctx.getNativeContext());
    if (cr) {
        int bx = x();
        int by = y();
        int bw = width();
        int bh = height();

        // Fondo gris sutilmente más oscuro para contraste premium
        cairo_set_source_rgb(cr, 0.94, 0.94, 0.94);
        cairo_rectangle(cr, bx, by, bw, bh);
        cairo_fill(cr);

        // Línea separadora a la derecha
        cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, bx + bw - 1.0, by);
        cairo_line_to(cr, bx + bw - 1.0, by + bh);
        cairo_stroke(cr);
    }

    // Llamar a ScrollArea::render para que gestione el clipping y renderice los hijos (miniaturas)
    ScrollArea::render(ctx, cx, cy, cw, ch, force);
}

void PdfSidebar::draw(horizon::GraphicsContext &ctx) {
    // Aquí solo dibujamos elementos que deben estar ENCIMA de todo (como los botones de scroll si se desea)
    // El fondo ya se dibujó en render()
    ScrollArea::draw(ctx);
}

} // namespace pdf
} // namespace horizon
