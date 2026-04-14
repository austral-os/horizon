#include "horizon/pdf/PdfThumbnailWidget.hpp"
#include <horizon/Logger.hpp>
#include <horizon/GraphicsContext.hpp>
#include <cairo.h>

namespace horizon {
namespace pdf {

PdfThumbnailWidget::PdfThumbnailWidget(int page_index) 
    : Widget(), m_page_index(page_index) {
    set_margin(10);
    set_width(m_thumb_width + 20); // Width + padding
    set_height(200); // Set actual height for ScrollArea detection
    set_fixed_size(200); // Set fixed size for layout weighting
    
    when_click.connect([this](horizon::MouseButtonEventContext& ev) {
        when_page_selected.run(m_page_index);
        ev.stop_propagation = true;
    });
}

void PdfThumbnailWidget::set_document(std::shared_ptr<PdfDocument> doc) {
    m_document = doc;
    invalidate();
}

void PdfThumbnailWidget::set_selected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        invalidate();
    }
}

void PdfThumbnailWidget::draw(horizon::GraphicsContext &ctx) {
    if (!m_document) return;

    int bx = x();
    int by = y();
    int bw = width();
    int bh = height();

    cairo_t* cr = static_cast<cairo_t*>(ctx.getNativeContext());
    if (!cr) return;

    // Obtener página de Poppler
    PopplerPage* page = m_document->get_page(m_page_index);
    if (!page) return;

    // Calcular dimensiones de la miniatura manteniendo aspecto
    double page_w, page_h;
    poppler_page_get_size(page, &page_w, &page_h);
    
    double aspect = page_h / page_w;
    int render_w = m_thumb_width;
    int render_h = (int)(render_w * aspect);
    
    // Centrar en el widget
    int fx = bx + (bw - render_w) / 2;
    int fy = by + 10;

    // Resalto de Selección
    if (m_selected) {
        cairo_set_source_rgba(cr, 0.0, 0.47, 0.85, 0.1);
        cairo_rectangle(cr, bx + 2, by + 2, bw - 4, bh - 4);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.0, 0.47, 0.85);
        cairo_set_line_width(cr, 2.0);
        cairo_rectangle(cr, bx + 2, by + 2, bw - 4, bh - 4);
        cairo_stroke(cr);
    }

    // Sombreado de la página
    cairo_set_source_rgba(cr, 0, 0, 0, 0.05);
    cairo_rectangle(cr, fx + 2, fy + 2, render_w, render_h);
    cairo_fill(cr);

    // Fondo blanco del papel
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, fx, fy, render_w, render_h);
    cairo_fill(cr);

    // Borde delimitador de la página
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, fx, fy, render_w, render_h);
    cairo_stroke(cr);

    // Renderizar contenido PDF
    cairo_save(cr);
    cairo_translate(cr, fx, fy);
    double scale = (double)render_w / page_w;
    cairo_scale(cr, scale, scale);
    
    // Usar Poppler directamente para renderizar
    poppler_page_render(page, cr);
    cairo_restore(cr);
    
    g_object_unref(page);
}

} // namespace pdf
} // namespace horizon
