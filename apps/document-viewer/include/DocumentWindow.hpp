#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/TabCollection.hpp>
#include <horizon/pdf/PdfWidget.hpp>
#include <horizon/pdf/PdfDocument.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/Label.hpp>
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
    horizon::Label* m_status_label{nullptr};
    horizon::ProgressBar* m_progress_bar{nullptr};
    
    // Actions
    void on_zoom_in();
    void on_zoom_out();
    void on_zoom_fit();
    void on_toggle_fullscreen();
    void on_toggle_sidebar();
    
    bool m_sidebar_visible{true};
    bool m_is_immersive{false};
};

} // namespace pdf
} // namespace horizon
