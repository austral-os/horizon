#include "horizon/pdf/PdfWidget.hpp"
#include <horizon/GraphicsContext.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/WaylandWindow.hpp>
#include <poppler.h>
#include <cairo.h>
#include <iostream>
#include <cmath>
#include <cstring>

namespace horizon {
namespace pdf {

PdfWidget::PdfWidget() : Widget(), m_page_spacing(20) {
    set_layout_type(WIDGET_LAYOUT_VERTICAL);
    set_cursor_type(CursorType::Text);
    
    when_mouse_press.connect([this](horizon::MouseButtonEventContext& ev) { handle_mouse_press(ev); });
    when_mouse_drag.connect([this](horizon::MouseMoveEventContext& ev) { handle_mouse_drag(ev); });
    when_mouse_release.connect([this](horizon::MouseButtonEventContext& ev) { handle_mouse_release(ev); });
}

PdfWidget::~PdfWidget() {}

void PdfWidget::set_document(std::shared_ptr<PdfDocument> doc) {
    m_document = doc;
    calculate_page_layout();
    invalidate();
}

void PdfWidget::calculate_page_layout() {
    if (!m_document) return;

    m_page_y_positions.clear();
    double current_y = m_page_spacing;
    double max_width = 0;
    int count = m_document->page_count();

    for (int i = 0; i < count; ++i) {
        m_page_y_positions.push_back((int)current_y);
        
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

int PdfWidget::get_page_at_y(int y) const {
    if (m_page_y_positions.empty()) return 0;
    
    // Ajustar Y para considerar el margen inicial
    // Buscamos la página cuya posición Y sea la más cercana a 'y' sin pasarse
    int last_idx = 0;
    for (size_t i = 0; i < m_page_y_positions.size(); ++i) {
        if (m_page_y_positions[i] > y + 50) { // Offset de 50px para dar margen de visualización
            break;
        }
        last_idx = (int)i;
    }
    return last_idx;
}

void PdfWidget::calculate_layout() {
    // Si tenemos un padre (ScrollArea), permitimos que el widget se ensanche
    // para cubrir todo el visor y permitir el centrado horizontal.
    if (parent()) {
        int target_w = std::max(m_total_width, parent()->width());
        if (width() != target_w) {
            set_width(target_w);
        }
    } else {
        set_width(m_total_width);
    }
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
    
    // Coordenadas absolutas en ventana
    int bx = x();
    int by = y();
    int bw = width();
    int bh = height();

    // Obtener área visible para renderizado selectivo
    int viewport_y = -by; 
    int viewport_h = 0;
    
    if (parent()) {
        viewport_h = parent()->height();
    } else {
        viewport_h = 800; // Fallback
    }

    // Dibujar fondo de documento que cubra TODO el widget (ahora posiblemente más ancho)
    ctx.setColor(Color(0.85f, 0.85f, 0.85f, 1.0f)); // Gris un poco más neutro estilo Nova
    ctx.fillRect(bx, by, bw, bh);

    double current_y = m_page_spacing;
    int count = m_document->page_count();

    for (int i = 0; i < count; ++i) {
        PopplerPage* page = m_document->get_page(i);
        if (page) {
            double w, h;
            poppler_page_get_size(page, &w, &h);

            // Verificar visibilidad
            bool is_visible = (current_y + h >= viewport_y) && (current_y <= viewport_y + viewport_h);

            if (is_visible) {
                // Centrado dinámico basado en el ANCHO TOTAL del widget (bw)
                double rx = std::max((double)m_page_spacing, (bw - w) / 2.0);
                
                double fx = bx + rx;
                double fy = by + current_y;

                // Sombra suave (Shadow)
                cairo_set_source_rgba(cr, 0, 0, 0, 0.15);
                cairo_rectangle(cr, fx + 2, fy + 2, w + 2, h + 2);
                cairo_fill(cr);

                // Fondo blanco
                cairo_set_source_rgb(cr, 1, 1, 1);
                cairo_rectangle(cr, fx, fy, w, h);
                cairo_fill(cr);

                // Renderizado Poppler
                cairo_save(cr);
                cairo_translate(cr, fx, fy);
                
                poppler_page_render(page, cr);

                // Dibujar Selección si corresponde a esta página (DESPUÉS del contenido)
                if (m_sel_page_idx == i) {
                    PopplerRectangle sel_rect;
                    sel_rect.x1 = std::min(m_sel_start_x, m_sel_end_x);
                    sel_rect.y1 = std::min(m_sel_start_y, m_sel_end_y);
                    sel_rect.x2 = std::max(m_sel_start_x, m_sel_end_x);
                    sel_rect.y2 = std::max(m_sel_start_y, m_sel_end_y);

                    // Color de selección estilo Nova (Azul transparente)
                    cairo_set_source_rgba(cr, 0.0, 0.47, 0.85, 0.3);
                    
                    cairo_region_t* region = poppler_page_get_selected_region(page, 1.0, POPPLER_SELECTION_GLYPH, &sel_rect);
                    if (region) {
                        int n_rects = cairo_region_num_rectangles(region);
                        for (int r = 0; r < n_rects; ++r) {
                            cairo_rectangle_int_t r_rect;
                            cairo_region_get_rectangle(region, r, &r_rect);
                            cairo_rectangle(cr, r_rect.x, r_rect.y, r_rect.width, r_rect.height);
                        }
                        cairo_fill(cr);
                        cairo_region_destroy(region);
                    }
                }
                
                cairo_restore(cr);

                // Borde suave
                cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
                cairo_set_line_width(cr, 1.0);
                cairo_rectangle(cr, fx, fy, w, h);
                cairo_stroke(cr);
            }

            current_y += h + m_page_spacing;
            g_object_unref(page);
        }
    }
}

void PdfWidget::handle_mouse_press(horizon::MouseButtonEventContext &ev) {
    if (!m_document) return;
    
    // Convertir a coordenadas locales
    int local_x = ev.x - x();
    int local_y = ev.y - y();
    
    m_sel_page_idx = get_page_at_y(local_y);
    m_is_selecting = true;
    
    // Coordenadas de página
    int bw = width();
    PopplerPage* page = m_document->get_page(m_sel_page_idx);
    if (page) {
        double w, h;
        poppler_page_get_size(page, &w, &h);
        double rx = std::max((double)m_page_spacing, (bw - w) / 2.0);
        double current_y = m_page_y_positions[m_sel_page_idx];
        
        m_sel_start_x = (local_x - rx);
        m_sel_start_y = (local_y - current_y);
        m_sel_end_x = m_sel_start_x;
        m_sel_end_y = m_sel_start_y;
        
        g_object_unref(page);
    }
    
    invalidate();
    ev.stop_propagation = true;
}

void PdfWidget::handle_mouse_drag(horizon::MouseMoveEventContext &ev) {
    if (!m_is_selecting || !m_document) return;
    
    int local_x = ev.x - x();
    int local_y = ev.y - y();
    
    int bw = width();
    PopplerPage* page = m_document->get_page(m_sel_page_idx);
    if (page) {
        double w, h;
        poppler_page_get_size(page, &w, &h);
        double rx = std::max((double)m_page_spacing, (bw - w) / 2.0);
        double current_y = m_page_y_positions[m_sel_page_idx];
        
        m_sel_end_x = (local_x - rx);
        m_sel_end_y = (local_y - current_y);
        
        g_object_unref(page);
    }
    
    invalidate();
    ev.stop_propagation = true;
}

void PdfWidget::handle_mouse_release(horizon::MouseButtonEventContext &ev) {
    m_is_selecting = false;
    ev.stop_propagation = true;
    // Mantenemos m_sel_page_idx y coordenadas para permitir copiar después
}

bool PdfWidget::can_perform(horizon::ClipboardAction action) const {
    return action == horizon::ClipboardAction::Copy && m_sel_page_idx != -1;
}

void PdfWidget::perform(horizon::ClipboardAction action) {
    if (action == horizon::ClipboardAction::Copy) {
        if (application()) {
            application()->set_clipboard_owner(this);
        }
    }
}

void PdfWidget::provide_clipboard_data(const std::string &mime, horizon::DataSink &sink) {
    if (mime == "text/plain" && m_document && m_sel_page_idx != -1) {
        PopplerPage* page = m_document->get_page(m_sel_page_idx);
        if (page) {
            PopplerRectangle sel_rect;
            sel_rect.x1 = std::min(m_sel_start_x, m_sel_end_x);
            sel_rect.y1 = std::min(m_sel_start_y, m_sel_end_y);
            sel_rect.x2 = std::max(m_sel_start_x, m_sel_end_x);
            sel_rect.y2 = std::max(m_sel_start_y, m_sel_end_y);
            
            char* text = poppler_page_get_selected_text(page, POPPLER_SELECTION_GLYPH, &sel_rect);
            if (text) {
                std::string s(text);
                sink.write(std::vector<uint8_t>(s.begin(), s.end()));
                g_free(text);
            }
            g_object_unref(page);
        }
    }
}

} // namespace pdf
} // namespace horizon
