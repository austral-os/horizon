#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/TabCollection.hpp>
#include <horizon/pdf/PdfWidget.hpp>
#include <horizon/pdf/PdfDocument.hpp>
#include <memory>

namespace horizon {
namespace pdf {

class DocumentWindow : public horizon::ApplicationWindow {
public:
    DocumentWindow();
    virtual ~DocumentWindow();

    void open_file(const std::string& path);

private:
    void build_toolbar();
    void build_content();
    
    // UI Components
    horizon::TabCollection* m_tabs{nullptr};
    
    // Actions
    void on_zoom_in();
    void on_zoom_out();
    void on_zoom_fit();
    void on_toggle_fullscreen();
};

} // namespace pdf
} // namespace horizon
