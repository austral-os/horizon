#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ScrollArea.hpp>
#include "horizon/pdf/PdfDocument.hpp"
#include <memory>
#include <vector>

namespace horizon {
namespace pdf {

class PdfSidebar : public horizon::Widget {
public:
    PdfSidebar();
    ~PdfSidebar() = default;

    void set_document(std::shared_ptr<PdfDocument> doc);
    
    // Signal for when a page is clicked in the sidebar
    horizon::EventsManager<int> when_page_selected;

private:
    std::shared_ptr<PdfDocument> m_document;
    horizon::Widget* m_list_container{nullptr};
};

} // namespace pdf
} // namespace horizon
