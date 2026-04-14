#include "horizon/pdf/PdfWidget.hpp"
#include <horizon/GraphicsContext.hpp>
#include <poppler.h>
#include <cairo.h>
#include <iostream>

namespace horizon {
namespace pdf {

PdfWidget::PdfWidget() : Widget(), m_page_spacing(20) {
    set_layout_type(WIDGET_LAYOUT_VERTICAL);
    set_position_type(horizon::FILL);
}

PdfWidget::~PdfWidget() {}

void PdfWidget::set_document(std::shared_ptr<PdfDocument> doc) {
    m_document = doc;
    calculate_page_layout();
    invalidate();
}

void PdfWidget::calculate_page_layout() {
    if (!m_document) return;

    double current_y = m_page_spacing;
    double max_width = 0;
    int count = m_document->page_count();

    for (int i = 0; i < count; ++i) {
        PopplerPage* page = m_document->get_page(i);
        if (page) {
            double w, h;
            poppler_page_get_size(page, &w, &h);
            if (w > max_width) max_width = w;
            current_y += h + m_page_spacing;
            g_object_unref(page);
        }
    }

    set_width((int)max_width + m_page_spacing * 2);
    set_height((int)current_y);
}

void PdfWidget::calculate_layout() {
}

int PdfWidget::get_page_y(int page_index) const {
    if (!m_document) return 0;
    
    double current_y = m_page_spacing;
    int count = m_document->page_count();
    
    for (int i = 0; i < count && i < page_index; ++i) {
        PopplerPage* page = m_document->get_page(i);
        if (page) {
            double w, h;
            poppler_page_get_size(page, &w, &h);
            current_y += h + m_page_spacing;
            g_object_unref(page);
        }
    }
    
    return (int)current_y;
}

void PdfWidget::draw(horizon::GraphicsContext &ctx) {
    if (!m_document) return;

    cairo_t* cr = static_cast<cairo_t*>(ctx.getNativeContext());
    if (!cr) return;
    
    // Dibujar fondo de documento
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_paint(cr);

    double current_y = m_page_spacing;
    int count = m_document->page_count();

    for (int i = 0; i < count; ++i) {
        PopplerPage* page = m_document->get_page(i);
        if (page) {
            double w, h;
            poppler_page_get_size(page, &w, &h);

            double x = (width() - w) / 2.0;

            cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
            cairo_rectangle(cr, x + 4, current_y + 4, w, h);
            cairo_fill(cr);

            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_rectangle(cr, x, current_y, w, h);
            cairo_fill(cr);

            cairo_save(cr);
            cairo_translate(cr, x, current_y);
            poppler_page_render(page, cr);
            cairo_restore(cr);

            current_y += h + m_page_spacing;
            g_object_unref(page);
        }
    }
}

} // namespace pdf
} // namespace horizon
