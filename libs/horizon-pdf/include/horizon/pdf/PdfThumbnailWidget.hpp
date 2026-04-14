#pragma once

#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include "horizon/pdf/PdfDocument.hpp"
#include <memory>

namespace horizon {
namespace pdf {

class PdfThumbnailWidget : public horizon::Widget {
public:
    PdfThumbnailWidget(int page_index);
    ~PdfThumbnailWidget() = default;

    void set_document(std::shared_ptr<PdfDocument> doc);
    
    void draw(horizon::GraphicsContext &ctx) override;
    
    horizon::EventsManager<int> when_page_selected;

private:
    std::shared_ptr<PdfDocument> m_document;
    int m_page_index;
    int m_thumb_width = 120;
    int m_thumb_height = 0;
};

} // namespace pdf
} // namespace horizon
