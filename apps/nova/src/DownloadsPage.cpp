#include "DownloadsPage.hpp"
#include "horizon/download/DownloadView.hpp"

namespace horizon {
namespace nova {

DownloadsPage::DownloadsPage() {
    set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
    set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
    setup_ui();
}

void DownloadsPage::setup_ui() {
    auto view = std::make_unique<download::DownloadView>();
    m_view = view.get();
    add_child(std::move(view));
}

void DownloadsPage::calculate_layout() {
    if (m_view) {
        m_view->set_size(width(), height());
    }
    horizon::Widget::calculate_layout();
}

} // namespace nova
} // namespace horizon
