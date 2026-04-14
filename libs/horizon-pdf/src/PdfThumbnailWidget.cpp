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
    set_height(200); // Initial estimated height to reserve space in layout
    
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

    // Ajustar altura del widget si es necesario
    if (m_thumb_height != render_h) {
        m_thumb_height = render_h;
        set_height(m_thumb_height + 20);
        invalidate();
    }

    // Coordenadas absolutas del widget
    int bx = x();
    int by = y();

    // Center horizontally in the widget
    int rx_offset = (width() - render_w) / 2;
    int ry_offset = 10;

    // Coordenadas finales de dibujo
    int fx = bx + rx_offset;
    int fy = by + ry_offset;

    cairo_t* cr = static_cast<cairo_t*>(ctx.getNativeContext());
    if (!cr) return;
    
    // Dibujar fondo sombreado muy suave (premium look)
    cairo_set_source_rgba(cr, 0, 0, 0, 0.05);
    cairo_rectangle(cr, fx + 2, fy + 2, render_w, render_h);
    cairo_fill(cr);

    // Fondo blanco del papel
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, fx, fy, render_w, render_h);
    cairo_fill(cr);

    // Renderizar PDF
    cairo_save(cr);
    cairo_translate(cr, fx, fy);
    cairo_scale(cr, scale, scale);
    poppler_page_render(page, cr);
    cairo_restore(cr);

    // Borde delimitador sutil
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_set_line_width(cr, 0.5);
    cairo_rectangle(cr, fx, fy, render_w, render_h);
    cairo_stroke(cr);

    g_object_unref(page);
}

} // namespace pdf
} // namespace horizon
