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
    
    // Devuelve la posición Y de una página para scroll
    int get_page_y(int page_index) const;

protected:
    bool supports_clipboard() const override { return true; }
    
private:
    std::shared_ptr<PdfDocument> m_document;
    int m_page_spacing = 20;
};

} // namespace pdf
} // namespace horizon
