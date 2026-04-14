#include "horizon/pdf/PdfDocument.hpp"
#include <iostream>

namespace horizon {
namespace pdf {

PdfDocument::PdfDocument(PopplerDocument* doc) : m_doc(doc) {}

PdfDocument::~PdfDocument() {
    if (m_doc) {
        g_object_unref(m_doc);
    }
}

std::unique_ptr<PdfDocument> PdfDocument::open(const std::string& path) {
    GError* error = nullptr;
    std::string uri = "file://" + path;
    PopplerDocument* doc = poppler_document_new_from_file(uri.c_str(), nullptr, &error);
    
    if (error) {
        std::cerr << "PdfDocument: Error opening " << path << ": " << error->message << std::endl;
        g_error_free(error);
        return nullptr;
    }
    
    return std::unique_ptr<PdfDocument>(new PdfDocument(doc));
}

int PdfDocument::page_count() const {
    return poppler_document_get_n_pages(m_doc);
}

PopplerPage* PdfDocument::get_page(int index) const {
    if (index < 0 || index >= page_count()) return nullptr;
    return poppler_document_get_page(m_doc, index);
}

std::string PdfDocument::get_title() const {
    char* title = nullptr;
    g_object_get(m_doc, "title", &title, NULL);
    std::string result = title ? title : "";
    g_free(title);
    return result;
}

std::string PdfDocument::get_author() const {
    char* author = nullptr;
    g_object_get(m_doc, "author", &author, NULL);
    std::string result = author ? author : "";
    g_free(author);
    return result;
}

} // namespace pdf
} // namespace horizon
