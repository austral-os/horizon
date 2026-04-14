#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
#include "horizon/pdf/PdfDocument.hpp"
#include <memory>
#include <vector>

namespace horizon {
namespace pdf {

class PdfSidebar : public horizon::ScrollArea {
public:
    PdfSidebar();
    ~PdfSidebar() = default;

    void set_document(std::shared_ptr<PdfDocument> doc);
    
    void render(horizon::GraphicsContext &ctx, int cx, int cy, int cw, int ch, bool force) override;
    void draw(horizon::GraphicsContext &ctx) override;
    
    // Signal for when a page is clicked in the sidebar
    horizon::EventsManager<int> when_page_selected;

private:
    std::shared_ptr<PdfDocument> m_document;
};

} // namespace pdf
} // namespace horizon
