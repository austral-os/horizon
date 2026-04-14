#include "horizon/pdf/PdfThumbnailWidget.hpp"
#include <horizon/GraphicsContext.hpp>
#include <poppler.h>
#include <cairo.h>

namespace horizon {
namespace pdf {

PdfThumbnailWidget::PdfThumbnailWidget(int page_index) 
    : Widget(), m_page_index(page_index) {
    set_margin(10);
    set_width(m_thumb_width + 20); // Width + padding
    
    when_click.connect([this](horizon::MouseButtonEventContext& ev) {
        when_page_selected.run(m_page_index);
    });
}

void PdfThumbnailWidget::set_document(std::shared_ptr<PdfDocument> doc) {
    m_document = doc;
}

void PdfThumbnailWidget::draw(horizon::GraphicsContext &ctx) {
    if (!m_document) return;

    PopplerPage* page = poppler_document_get_page(m_document->raw(), m_page_index);
    if (!page) return;

    double pg_w, pg_h;
    poppler_page_get_size(page, &pg_w, &pg_h);

    double scale = (double)m_thumb_width / pg_w;
    int render_w = m_thumb_width;
    int render_h = (int)(pg_h * scale);

    // Ajustar altura del widget si es necesario (el padre manejará el layout)
    if (m_thumb_height != render_h) {
        m_thumb_height = render_h;
        set_height(m_thumb_height + 20);
        invalidate();
    }

    cairo_t* cr = static_cast<cairo_t*>(ctx.getNativeContext());
    
    // Dibujar fondo sombreado suave
    cairo_set_source_rgba(cr, 0, 0, 0, 0.15);
    cairo_rectangle(cr, 12, 12, render_w, render_h);
    cairo_fill(cr);

    // Fondo blanco del papel
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 10, 10, render_w, render_h);
    cairo_fill(cr);

    // Renderizar PDF
    cairo_save(cr);
    cairo_translate(cr, 10, 10);
    cairo_scale(cr, scale, scale);
    poppler_page_render(page, cr);
    cairo_restore(cr);

    // Borde delimitador
    cairo_set_source_rgb(cr, 0.75, 0.75, 0.75);
    cairo_set_line_width(cr, 1);
    cairo_rectangle(cr, 10, 10, render_w, render_h);
    cairo_stroke(cr);

    g_object_unref(page);
}

} // namespace pdf
} // namespace horizon
