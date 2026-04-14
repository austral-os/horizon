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

protected:
    bool supports_clipboard() const override { return true; }
    
private:
    std::shared_ptr<PdfDocument> m_document;
    std::vector<int> m_page_y_positions;
    int m_page_spacing = 20;
    int m_total_width{0};
    int m_total_height{0};
};

} // namespace pdf
} // namespace horizon
