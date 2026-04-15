#pragma once

#include <horizon/Widget.hpp>
#include "horizon/pdf/PdfDocument.hpp"
#include <memory>
#include <vector>

namespace horizon {
namespace pdf {

class PdfWidget : public horizon::Widget {
public:
    PdfWidget();
    ~PdfWidget();

    void set_document(std::shared_ptr<PdfDocument> doc);
    void calculate_page_layout();
    
    // Core widget overrides
    void draw(horizon::GraphicsContext &ctx) override;
    void calculate_layout() override;
    
    int preferred_width() const override;
    int preferred_height() const override;
    
    // Devuelve la posición Y de una página para scroll
    int get_page_y(int page_index) const;
    
    // Devuelve el índice de la página más visible en una coordenada Y
    int get_page_at_y(int y) const;
    
    // Controles de portapapeles
    bool supports_clipboard() const override { return true; }
    bool supports_fullscreen() const override { return true; }
    bool can_perform(horizon::ClipboardAction action) const override;
    void perform(horizon::ClipboardAction action) override;
    void provide_clipboard_data(const std::string &mime, horizon::DataSink &sink) override;
    std::vector<std::string> provided_mime_types() const override { return {"text/plain"}; }

protected:
    
private:
    std::shared_ptr<PdfDocument> m_document;
    std::vector<int> m_page_y_positions;
    int m_page_spacing = 20;
    int m_total_width{0};
    int m_total_height{0};
    
    // Estado de selección
    bool m_is_selecting{false};
    int m_sel_page_idx{-1};
    double m_sel_start_x{0}, m_sel_start_y{0};
    double m_sel_end_x{0}, m_sel_end_y{0};
    
    void handle_mouse_press(horizon::MouseButtonEventContext &ev);
    void handle_mouse_drag(horizon::MouseMoveEventContext &ev);
    void handle_mouse_release(horizon::MouseButtonEventContext &ev);
};

} // namespace pdf
} // namespace horizon
