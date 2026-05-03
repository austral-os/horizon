#pragma once
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/capture/CaptureEngine.h>
#include <horizon/capture/VideoRecorder.h>
#include <horizon/ConfigManager.hpp>
#include <memory>

namespace horizon::capture {

class CaptureWindow : public ApplicationWindow {
public:
    CaptureWindow();
    virtual ~CaptureWindow();

    void capture_screen_image();
    void capture_selection_image();
    void capture_window_image();

    void start_screen_video();
    void start_selection_video();
    void start_window_video();
    void stop_video();

private:
    void setup_ui();
    void setup_menus();

    std::shared_ptr<VideoRecorder> m_recorder;
    CaptureEngine m_engine;
    std::unique_ptr<ConfigManager> m_config;
    
    Toolbar* m_toolbar = nullptr;
    Label* m_status_label = nullptr;
    ToolbarButton* m_record_btn = nullptr;
    bool m_is_recording = false;
};

} // namespace horizon::capture
