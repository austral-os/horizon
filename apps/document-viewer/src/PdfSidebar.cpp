#include "PdfSidebar.hpp"
#include "horizon/pdf/PdfThumbnailWidget.hpp"
#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Logger.hpp>

namespace horizon {
namespace pdf {

PdfSidebar::PdfSidebar() : Widget() {
    set_layout_type(WIDGET_LAYOUT_VERTICAL);
    set_width(160); // Sidebar fixed width
    
    // Create ScrollArea
    auto scroll = std::make_unique<horizon::ScrollArea>();
    scroll->set_position_type(horizon::FILL);
    
    // Create Container for list
    auto container = std::make_unique<horizon::Widget>();
    container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    container->set_width(150);
    container->set_spacing(15);
    container->set_margin(10);
    m_list_container = container.get();
    
    scroll->set_content(std::move(container));
    add_child(std::move(scroll));
}

void PdfSidebar::set_document(std::shared_ptr<PdfDocument> doc) {
    m_document = doc;
    if (!m_list_container || !m_document) return;
    
    LOG_INFO << "PdfSidebar: Populating thumbnails...";
    
    m_list_container->clear_children();
    
    int page_count = m_document->page_count();
    for (int i = 0; i < page_count; ++i) {
        auto thumb = std::make_unique<PdfThumbnailWidget>(i);
        thumb->set_document(m_document);
        
        // Connect selection signal
        thumb->when_page_selected.connect([this](int index) {
            this->when_page_selected.run(index);
        });
        
        m_list_container->add_child(std::move(thumb));
    }
    
    invalidate();
}

} // namespace pdf
} // namespace horizon
