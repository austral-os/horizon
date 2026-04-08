#include "BrowserWindow.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Spacer.hpp"
#include <memory>

namespace horizon {
namespace nova {

BrowserWindow::BrowserWindow() : ApplicationWindow("Nova Web Browser") {
    set_size(1024, 768);
    setup_ui();
}

void BrowserWindow::setup_ui() {
    // 1. Nova Toolbar
    auto nova_toolbar = std::make_unique<NovaToolbar>();
    m_toolbar = nova_toolbar.get();
    toolbar()->add_toolbar_widget(std::move(nova_toolbar));

    // 2. Status Bar
    show_status_bar();
    auto* sb = statusbar();
    
    auto status_lbl = std::make_unique<horizon::Label>("Ready");
    m_status_label = status_lbl.get();

    auto pb_container = std::make_unique<horizon::Widget>();
    pb_container->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
    pb_container->set_fixed_size(200);

    auto pb = std::make_unique<horizon::ProgressBar>();
    m_progress_bar = pb.get();
    m_progress_bar->set_visible(false);
    m_progress_bar->set_fixed_size(10); // Height

    pb_container->add_child(horizon::Spacer());
    pb_container->add_child(std::move(pb));
    pb_container->add_child(horizon::Spacer());

    sb->add_child(horizon::Spacer(10));
    sb->add_child(std::move(status_lbl));
    sb->add_child(horizon::Spacer()); // Push to right
    sb->add_child(std::move(pb_container));
    sb->add_child(horizon::Spacer(10));

    // 3. Web View
    auto web_view = std::make_unique<web::WebWidget>();
    m_web_view = web_view.get();
    m_web_view->set_position_type(FILL);
    
    // --- Connections ---

    // Toolbar -> Web View
    m_toolbar->when_navigation_clicked.connect([this](NavigationButtonClickEvent& ctx) {
        if (!m_web_view) return;
        if (ctx.index == 0) m_web_view->go_back();
        else m_web_view->go_forward();
    });

    m_toolbar->when_home_clicked.connect([this](HomeButtonClickEvent&) {
        this->navigate_to_url("https://www.google.com");
    });

    m_toolbar->when_search_submitted.connect([this](SearchChangedEvent& ctx) {
        this->navigate_to_url(ctx.query);
    });

    m_toolbar->when_bookmark_clicked.connect([this](BookmarkButtonClickEvent&) {
        // TODO: Show bookmarks dialog
        LOG_INFO << "[NOVA] Bookmarks clicked";
    });

    m_toolbar->when_options_clicked.connect([this](OptionsButtonClickEvent&) {
        // TODO: Show options menu
        LOG_INFO << "[NOVA] Options clicked";
    });

    // Web View -> UI
    m_web_view->when_url_changed.connect([this](const std::string& url) {
        if (m_toolbar) m_toolbar->set_url(url);
    });

    m_web_view->when_title_changed.connect([this](const std::string& title) {
        if (!title.empty()) set_title(title + " - Nova");
        else set_title("Nova Web Browser");
    });

    m_web_view->when_loading_changed.connect([this](bool loading) {
        if (m_status_label) m_status_label->set_text(loading ? "Loading..." : "Done");
        if (m_progress_bar) m_progress_bar->set_visible(loading);
    });

    m_web_view->when_progress_changed.connect([this](double progress) {
        if (m_progress_bar) m_progress_bar->set_progress((float)progress);
    });

    // Initial load
    when_application_load.connect([this](auto&) {
        if (m_web_view) {
            m_web_view->load_url("https://www.google.com");
        }
    });

    set_content(std::move(web_view));
}

void BrowserWindow::navigate_to_url(const std::string& input_url) {
    if (m_web_view) {
        std::string url = input_url;
        if (url.empty()) return;
        
        // Basic URL normalization
        if (url.find("://") == std::string::npos) {
            // Check if it looks like a domain or search query
            if (url.find(".") != std::string::npos && url.find(" ") == std::string::npos) {
                url = "https://" + url;
            } else {
                url = "https://www.google.com/search?q=" + url;
            }
        }
        
        LOG_INFO << "[NOVA] Navigating to: " << url;
        m_web_view->load_url(url);
    }
}

} // namespace nova
} // namespace horizon
