#include "horizon/pdf/PdfWidget.hpp"
#include <horizon/GraphicsContext.hpp>
#include <horizon/ScrollArea.hpp>
#include <poppler.h>
#include <cairo.h>
#include <iostream>
#include <cmath>

namespace horizon {
namespace pdf {

PdfWidget::PdfWidget() : Widget(), m_page_spacing(20) {
    set_layout_type(WIDGET_LAYOUT_VERTICAL);
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

    m_total_width = (int)max_width + m_page_spacing * 2;
    m_total_height = (int)current_y;
    
    set_width(m_total_width);
    set_height(m_total_height);
}

void PdfWidget::calculate_layout() {
    // Aseguramos que el tamaño se mantenga según lo calculado
    set_width(m_total_width);
    set_height(m_total_height);
}

int PdfWidget::preferred_width() const {
    return m_total_width;
}

int PdfWidget::preferred_height() const {
    return m_total_height;
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
    
    // El widget usa coordenadas absolutas en la ventana
    int bx = x();
    int by = y();

    // Obtener área visible para renderizado selectivo
    // viewport_y es relativo al inicio del documento
    int viewport_y = -by; 
    int viewport_h = 0;
    
    if (parent()) {
        viewport_h = parent()->height();
    } else {
        viewport_h = 800; // Fallback
    }

    // Dibujar fondo de documento suave (sustituye a los bordes negros)
    ctx.setColor(Color(0.9f, 0.9f, 0.9f, 1.0f));
    ctx.fillRect(bx, by, width(), height());

    double current_y = m_page_spacing;
    int count = m_document->page_count();

    for (int i = 0; i < count; ++i) {
        PopplerPage* page = m_document->get_page(i);
        if (page) {
            double w, h;
            poppler_page_get_size(page, &w, &h);

            // Verificar si la página es visible en el viewport actual
            bool is_visible = (current_y + h >= viewport_y) && (current_y <= viewport_y + viewport_h);

            if (is_visible) {
                // Centrar página con un margen mínimo m_page_spacing relativo al widget
                double rx = std::max((double)m_page_spacing, (width() - w) / 2.0);
                
                // Coordenadas absolutas finales
                double fx = bx + rx;
                double fy = by + current_y;

                // Sombra de la página
                cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
                cairo_rectangle(cr, fx + 4, fy + 4, w, h);
                cairo_fill(cr);

                // Fondo blanco de la página
                cairo_set_source_rgb(cr, 1, 1, 1);
                cairo_rectangle(cr, fx, fy, w, h);
                cairo_fill(cr);

                // Renderizado de Poppler
                cairo_save(cr);
                cairo_translate(cr, fx, fy);
                poppler_page_render(page, cr);
                cairo_restore(cr);

                // Borde de la página
                cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
                cairo_set_line_width(cr, 0.5);
                cairo_rectangle(cr, fx, fy, w, h);
                cairo_stroke(cr);
            }

            current_y += h + m_page_spacing;
            g_object_unref(page);
        }
    }
}

} // namespace pdf
} // namespace horizon
