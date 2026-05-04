#include "CaptureWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/WaylandWindow.hpp>
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

    when_application_load.connect([this](EventContext&) {
        auto* app = application();
        if (!app) return;

        LOG_INFO << "[CaptureApp] Setting up global menu signal connections";

        app->signal_manager.connect("img_selection", [this](const SignalContext&) {
            LOG_INFO << "[CaptureApp] Global Signal: img_selection received";
            this->capture_selection_image();
        });
        app->signal_manager.connect("img_window", [this](const SignalContext&) {
            LOG_INFO << "[CaptureApp] Global Signal: img_window received";
            this->capture_window_image();
        });
        app->signal_manager.connect("img_screen", [this](const SignalContext&) {
            LOG_INFO << "[CaptureApp] Global Signal: img_screen received";
            this->capture_screen_image();
        });
        app->signal_manager.connect("vid_selection", [this](const SignalContext&) {
            LOG_INFO << "[CaptureApp] Global Signal: vid_selection received";
            this->start_selection_video();
        });
        app->signal_manager.connect("vid_window", [this](const SignalContext&) {
            LOG_INFO << "[CaptureApp] Global Signal: vid_window received";
            this->start_window_video();
        });
        app->signal_manager.connect("vid_screen", [this](const SignalContext&) {
            LOG_INFO << "[CaptureApp] Global Signal: vid_screen received";
            this->start_screen_video();
        });
    });
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

    auto title = std::make_unique<Label>(horizon::i18n().tr("capture.title"));
    title->set_fixed_size(32);
    // title->set_bold(true); // If supported
    content_panel->add_child(std::move(title));

    auto subtitle = std::make_unique<Label>(horizon::i18n().tr("capture.about.description"));
    content_panel->add_child(std::move(subtitle));

    m_status_label = new Label(horizon::i18n().tr("capture.status.ready"));
    auto status_ptr = std::unique_ptr<Label>(m_status_label);
    content_panel->add_child(std::move(status_ptr));

    // Toolbar setup
    m_toolbar = toolbar();
    if (m_toolbar) {
        m_toolbar->set_bottom_height(58);
        auto img_btn_ptr = std::make_unique<ToolbarButton>(horizon::i18n().tr("capture.toolbar.screenshot"), "camera-photo-symbolic");
        auto* img_btn = img_btn_ptr.get();
        m_toolbar->add_toolbar_widget(std::move(img_btn_ptr));
        img_btn->when_click.connect([this](const MouseButtonEventContext&) {
            capture_screen_image();
        });

        auto vid_btn_ptr = std::make_unique<ToolbarButton>(horizon::i18n().tr("capture.toolbar.record"), "camera-video-symbolic");
        m_record_btn = vid_btn_ptr.get();
        m_toolbar->add_toolbar_widget(std::move(vid_btn_ptr));
        m_record_btn->when_click.connect([this](const MouseButtonEventContext&) {
            if (m_is_recording) stop_video();
            else start_screen_video();
        });

        auto pref_btn_ptr = std::make_unique<ToolbarButton>(horizon::i18n().tr("capture.toolbar.preferences"), "preferences-system-symbolic");
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

std::string expand_tilde(std::string path) {
    if (path.empty() || path[0] != '~') return path;
    const char *home = std::getenv("HOME");
    if (!home) return path;
    if (path.size() == 1) return home;
    if (path[1] == '/') return std::string(home) + path.substr(1);
    return path;
}

void CaptureWindow::capture_screen_image() {
    m_config->load(); // Reload in case preferences changed
    
    std::string out_dir = expand_tilde(m_config->get_value("general", "output_directory", "."));
    std::string format = m_config->get_value("image", "format", "png");

    // Ensure directory exists
    if (!out_dir.empty() && !std::filesystem::exists(out_dir)) {
        std::filesystem::create_directories(out_dir);
    }
    
    LOG_INFO << "[CaptureApp] Starting screen capture...";
    m_status_label->set_text(horizon::i18n().tr("capture.status.capturing_screen"));
    
    std::string filename = "screenshot_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." + format;
    std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;
    
    LOG_INFO << "[CaptureApp] Saving to: " << full_path.string();
    
    if (m_engine.capture_screenshot("", full_path.string())) {
        LOG_INFO << "[CaptureApp] Screenshot successful";
        m_status_label->set_text(horizon::i18n().tr("capture.status.saved").replace(horizon::i18n().tr("capture.status.saved").find("{}"), 2, full_path.string()));
    } else {
        LOG_ERROR << "[CaptureApp] Screenshot failed";
        m_status_label->set_text(horizon::i18n().tr("capture.status.failed"));
    }
}

void CaptureWindow::capture_selection_image() {
    LOG_INFO << "[CaptureApp] capture_selection_image() called";
    m_status_label->set_text(horizon::i18n().tr("capture.status.select_region"));
    m_selection_win = std::make_shared<SelectionWindow>();

    std::thread([win = m_selection_win]() {
        win->initialize();
        win->set_visible(true);
        win->run();
    }).detach();
    
    m_selection_win->selection_widget()->when_selected().connect([this](SelectionRect rect) {
        int x = rect.x + m_selection_win->screen_x();
        int y = rect.y + m_selection_win->screen_y();
        int w = rect.width;
        int h = rect.height;
        
        m_selection_win->set_visible(false);
        m_selection_win->quit();
        this->application()->post_task([this]() {
            m_selection_win.reset();
        });

        // Small delay to ensure the window is gone from the compositor's view
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        m_config->load();
        std::string out_dir = expand_tilde(m_config->get_value("general", "output_directory", "."));
        std::string format = m_config->get_value("image", "format", "png");

        if (!out_dir.empty() && !std::filesystem::exists(out_dir)) {
            std::filesystem::create_directories(out_dir);
        }
        
        std::string filename = "screenshot_selection_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." + format;
        std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;
        
        if (m_engine.capture_region("", x, y, w, h, full_path.string())) {
            m_status_label->set_text(horizon::i18n().tr("capture.status.saved").replace(horizon::i18n().tr("capture.status.saved").find("{}"), 2, full_path.string()));
        } else {
            m_status_label->set_text(horizon::i18n().tr("capture.status.failed"));
        }
    });

    m_selection_win->selection_widget()->when_cancelled().connect([this](const EventContext&) {
        LOG_INFO << "[CaptureApp] Selection cancelled";
        m_selection_win->set_visible(false);
        m_selection_win->quit();
        this->application()->post_task([this]() {
            m_selection_win.reset();
        });
        m_status_label->set_text(horizon::i18n().tr("capture.status.ready"));
    });
}

void CaptureWindow::capture_window_image() {
    m_status_label->set_text(horizon::i18n().tr("capture.status.not_implemented"));
    application()->alert(horizon::i18n().tr("capture.alert.not_implemented_msg"), horizon::i18n().tr("capture.alert.not_implemented_title"));
}

void CaptureWindow::start_screen_video() {
    if (m_is_recording) return;
    
    m_config->load();
    std::string out_dir = expand_tilde(m_config->get_value("general", "output_directory", "."));
    std::string container = m_config->get_value("video", "container", "mp4");
    
    // Ensure directory exists
    if (!out_dir.empty() && !std::filesystem::exists(out_dir)) {
        std::filesystem::create_directories(out_dir);
    }
    
    m_status_label->set_text(horizon::i18n().tr("capture.status.recording_screen"));
    std::string filename = "recording_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." + container;
    std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;
    
    // Get monitor dimensions (dummy way for now)
    int w = 1920, h = 1080; 
    
    LOG_INFO << "[CaptureApp] Starting video recording to: " << full_path.string();
    
    if (m_recorder->start(full_path.string(), 0, 0, w, h, 30, true)) {
        m_is_recording = true;
        m_status_label->set_text(horizon::i18n().tr("capture.status.recording_stop_hint"));
        if (m_record_btn) {
            m_record_btn->set_title(horizon::i18n().tr("capture.toolbar.stop"));
            m_record_btn->set_icon_name("media-playback-stop-symbolic");
        }
    } else {
        m_status_label->set_text(horizon::i18n().tr("capture.status.failed"));
    }
}

void CaptureWindow::start_selection_video() {
    LOG_INFO << "[CaptureApp] start_selection_video() called";
    m_status_label->set_text(horizon::i18n().tr("capture.status.select_region"));
    m_selection_win = std::make_shared<SelectionWindow>();

    std::thread([win = m_selection_win]() {
        win->initialize();
        win->set_visible(true);
        win->run();
    }).detach();
    
    m_selection_win->selection_widget()->when_selected().connect([this](SelectionRect rect) {
        int x = rect.x + m_selection_win->screen_x();
        int y = rect.y + m_selection_win->screen_y();
        int w = rect.width;
        int h = rect.height;
        
        if (w % 2 != 0) w--;
        if (h % 2 != 0) h--;

        m_selection_win->set_visible(false);
        m_selection_win->quit();
        this->application()->post_task([this]() {
            m_selection_win.reset();
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        m_config->load();
        std::string out_dir = expand_tilde(m_config->get_value("general", "output_directory", "."));
        std::string container = m_config->get_value("video", "container", "mp4");
        
        if (!out_dir.empty() && !std::filesystem::exists(out_dir)) {
            std::filesystem::create_directories(out_dir);
        }
        
        std::string filename = "recording_selection_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." + container;
        std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;
        
        LOG_INFO << "[CaptureApp] Starting region recording to: " << full_path.string();
        
        if (m_recorder->start(full_path.string(), x, y, w, h, 30, true)) {
            m_is_recording = true;
            m_status_label->set_text(horizon::i18n().tr("capture.status.recording_screen"));
            if (m_record_btn) {
                m_record_btn->set_title(horizon::i18n().tr("capture.toolbar.stop"));
                m_record_btn->set_icon_name("media-playback-stop-symbolic");
            }
        } else {
            m_status_label->set_text(horizon::i18n().tr("capture.status.failed"));
        }
    });

    m_selection_win->selection_widget()->when_cancelled().connect([this](const EventContext&) {
        LOG_INFO << "[CaptureApp] Selection cancelled";
        m_selection_win->set_visible(false);
        m_selection_win->quit();
        this->application()->post_task([this]() {
            m_selection_win.reset();
        });
        m_status_label->set_text(horizon::i18n().tr("capture.status.ready"));
    });
}

void CaptureWindow::start_window_video() {
    m_status_label->set_text(horizon::i18n().tr("capture.status.not_implemented"));
    application()->alert(horizon::i18n().tr("capture.alert.not_implemented_msg"), horizon::i18n().tr("capture.alert.not_implemented_title"));
}

void CaptureWindow::stop_video() {
    if (!m_is_recording) return;
    m_recorder->stop();
    m_is_recording = false;
    m_status_label->set_text(horizon::i18n().tr("capture.status.recording_finished"));
    if (m_record_btn) {
        m_record_btn->set_title(horizon::i18n().tr("capture.toolbar.record"));
        m_record_btn->set_icon_name("camera-video-symbolic");
    }
}

} // namespace horizon::capture
