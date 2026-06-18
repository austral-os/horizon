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
#include <horizon/Combo.hpp>
#include <horizon/HPanel.hpp>
#include <functional>

namespace horizon::capture {

class SelectionWindow;

class CaptureWindow : public ApplicationWindow {
public:
    CaptureWindow();
    virtual ~CaptureWindow();

    void capture_screen_image(const std::string& output_name = "");
    void capture_selection_image();
    void capture_window_image();
    
    void set_quit_on_finish(bool quit) { m_quit_on_finish = quit; }
    void set_edit_on_finish(bool edit) { m_edit_on_finish = edit; }

    void start_screen_video(const std::string& output_name = "");
    void start_selection_video();
    void start_window_video();
    void stop_video();

private:
    void setup_ui();
    void setup_menus();
    void execute_with_delay(std::function<void()> action);

    std::shared_ptr<VideoRecorder> m_recorder;
    CaptureEngine m_engine;
    std::unique_ptr<ConfigManager> m_config;
    std::shared_ptr<SelectionWindow> m_selection_win;
    
    Toolbar* m_toolbar = nullptr;
    Label* m_status_label = nullptr;
    ToolbarButton* m_record_btn = nullptr;
    Combo* m_delay_combo = nullptr;
    Combo* m_target_combo = nullptr;
    bool m_is_recording = false;
    
    int m_delay_seconds = 0;
    size_t m_countdown_timer_id = 0;
    int m_current_countdown = 0;
    
    bool m_quit_on_finish = false;
    bool m_edit_on_finish = false;
};

} // namespace horizon::capture
