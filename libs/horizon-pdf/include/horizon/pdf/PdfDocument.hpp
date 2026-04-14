#pragma once

#include <string>
#include <vector>
#include <memory>
#include <poppler.h>

namespace horizon {
namespace pdf {

class PdfDocument {
public:
    static std::unique_ptr<PdfDocument> open(const std::string& path);
    ~PdfDocument();

    int page_count() const;
    PopplerPage* get_page(int index) const;
    PopplerDocument* raw() const { return m_doc; }
    
    std::string get_title() const;
    std::string get_author() const;

private:
    PdfDocument(PopplerDocument* doc);
    PopplerDocument* m_doc;
};

} // namespace pdf
} // namespace horizon
