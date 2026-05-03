#include "CaptureWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/MenuBar.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/Label.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Logger.hpp>
#include <horizon/I18n.hpp>
#include <horizon/capture/SelectionWindow.h>
#include <horizon/ApplicationLauncher.hpp>
#include <chrono>
#include <thread>

namespace horizon::capture {

CaptureWindow::CaptureWindow() : ApplicationWindow("Capture") {
    m_recorder = std::make_shared<VideoRecorder>();
    if (!m_engine.init()) {
        LOG_ERROR << "[CaptureApp] Failed to initialize CaptureEngine!";
    }

    char *home = std::getenv("HOME");
    std::string config_path = home ? std::string(home) + "/.config/horizon/capture.json" : "capture.json";
    m_config = std::make_unique<ConfigManager>(config_path);
    m_config->load();
    
    setup_ui();
}

CaptureWindow::~CaptureWindow() {
    if (m_is_recording) {
        stop_video();
    }
}

void CaptureWindow::setup_ui() {
    auto content_panel = std::make_unique<VPanel>();
    content_panel->set_spacing(20);
    content_panel->set_margin(40);
    content_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);

    auto title = std::make_unique<Label>("Horizon Capture");
    title->set_fixed_size(32);
    // title->set_bold(true); // If supported
    content_panel->add_child(std::move(title));

    auto subtitle = std::make_unique<Label>("Capture your screen, windows or specific regions.");
    content_panel->add_child(std::move(subtitle));

    m_status_label = new Label("Ready");
    auto status_ptr = std::unique_ptr<Label>(m_status_label);
    content_panel->add_child(std::move(status_ptr));

    // Toolbar setup
    m_toolbar = toolbar();
    if (m_toolbar) {
        auto img_btn_ptr = std::make_unique<ToolbarButton>("Screenshot", "camera-photo-symbolic");
        auto* img_btn = img_btn_ptr.get();
        m_toolbar->add_toolbar_widget(std::move(img_btn_ptr));
        img_btn->when_click.connect([this](const MouseButtonEventContext&) {
            capture_screen_image();
        });

        auto vid_btn_ptr = std::make_unique<ToolbarButton>("Record Video", "camera-video-symbolic");
        m_record_btn = vid_btn_ptr.get();
        m_toolbar->add_toolbar_widget(std::move(vid_btn_ptr));
        m_record_btn->when_click.connect([this](const MouseButtonEventContext&) {
            if (m_is_recording) stop_video();
            else start_screen_video();
        });

        auto pref_btn_ptr = std::make_unique<ToolbarButton>("Preferences", "preferences-system-symbolic");
        auto* pref_btn = pref_btn_ptr.get();
        m_toolbar->add_toolbar_widget(std::move(pref_btn_ptr));
        pref_btn->when_click.connect([this](const MouseButtonEventContext&) {
            if (this->application()) {
                this->application()->show_preferences();
            }
        });
    }

    set_content(std::move(content_panel));
    set_size(600, 400);
}

void CaptureWindow::capture_screen_image() {
    m_config->load(); // Reload in case preferences changed
    
    std::string out_dir = m_config->get_value("general", "output_directory", ".");
    std::string format = m_config->get_value("image", "format", "png");
    
    LOG_INFO << "[CaptureApp] Starting screen capture...";
    m_status_label->set_text("Capturing screen...");
    
    std::string filename = "screenshot_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." + format;
    std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;
    
    LOG_INFO << "[CaptureApp] Saving to: " << full_path.string();
    
    if (m_engine.capture_screenshot("", full_path.string())) {
        LOG_INFO << "[CaptureApp] Screenshot successful";
        m_status_label->set_text("Screenshot saved to " + full_path.string());
    } else {
        LOG_ERROR << "[CaptureApp] Screenshot failed";
        m_status_label->set_text("Failed to capture screenshot");
    }
}

void CaptureWindow::capture_selection_image() {
    m_status_label->set_text("Select region...");
    auto selection_win = std::make_shared<SelectionWindow>();
    selection_win->initialize();
    
    selection_win->selection_widget()->when_selected().connect([this, selection_win](SelectionRect rect) {
        int x = rect.x + selection_win->screen_x();
        int y = rect.y + selection_win->screen_y();
        int w = rect.width;
        int h = rect.height;
        
        selection_win->set_visible(false);
        
        // Brief delay to let selection window disappear
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        m_config->load();
        std::string out_dir = m_config->get_value("general", "output_directory", ".");
        std::string format = m_config->get_value("image", "format", "png");
        
        std::string filename = "screenshot_selection_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." + format;
        std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;
        
        if (m_engine.capture_region("", x, y, w, h, full_path.string())) {
            m_status_label->set_text("Region saved to " + full_path.string());
        } else {
            m_status_label->set_text("Failed to capture region");
        }
        
        selection_win->quit();
    });
    
    selection_win->run();
}

void CaptureWindow::capture_window_image() {
    m_status_label->set_text("Window capture not implemented yet");
}

void CaptureWindow::start_screen_video() {
    if (m_is_recording) return;
    
    m_config->load();
    std::string out_dir = m_config->get_value("general", "output_directory", ".");
    std::string container = m_config->get_value("video", "container", "mp4");
    
    m_status_label->set_text("Recording screen...");
    std::string filename = "recording_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." + container;
    std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;
    
    // Get monitor dimensions (dummy way for now)
    int w = 1920, h = 1080; 
    
    LOG_INFO << "[CaptureApp] Starting video recording to: " << full_path.string();
    
    if (m_recorder->start(full_path.string(), 0, 0, w, h, 30, true)) {
        m_is_recording = true;
        m_status_label->set_text("Recording... (Press Stop to finish)");
        if (m_record_btn) {
            m_record_btn->set_title("Stop");
            m_record_btn->set_icon_name("media-playback-stop-symbolic");
        }
    } else {
        m_status_label->set_text("Failed to start recording");
    }
}

void CaptureWindow::start_selection_video() {
    m_status_label->set_text("Select region for video...");
    auto selection_win = std::make_shared<SelectionWindow>();
    selection_win->initialize();
    
    selection_win->selection_widget()->when_selected().connect([this, selection_win](SelectionRect rect) {
        int x = rect.x + selection_win->screen_x();
        int y = rect.y + selection_win->screen_y();
        int w = rect.width;
        int h = rect.height;
        
        if (w % 2 != 0) w--;
        if (h % 2 != 0) h--;

        selection_win->set_visible(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        m_config->load();
        std::string out_dir = m_config->get_value("general", "output_directory", ".");
        std::string container = m_config->get_value("video", "container", "mp4");
        
        std::string filename = "recording_selection_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." + container;
        std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;
        
        LOG_INFO << "[CaptureApp] Starting region recording to: " << full_path.string();
        
        if (m_recorder->start(full_path.string(), x, y, w, h, 30, true)) {
            m_is_recording = true;
            m_status_label->set_text("Recording region...");
            if (m_record_btn) {
                m_record_btn->set_title("Stop");
                m_record_btn->set_icon_name("media-playback-stop-symbolic");
            }
        } else {
            m_status_label->set_text("Failed to start region recording");
        }
        
        selection_win->quit();
    });
    
    selection_win->run();
}

void CaptureWindow::start_window_video() {
    m_status_label->set_text("Window video recording not implemented yet");
}

void CaptureWindow::stop_video() {
    if (!m_is_recording) return;
    m_recorder->stop();
    m_is_recording = false;
    m_status_label->set_text("Recording finished and saved.");
    if (m_record_btn) {
        m_record_btn->set_title("Record Video");
        m_record_btn->set_icon_name("camera-video-symbolic");
    }
}

} // namespace horizon::capture
