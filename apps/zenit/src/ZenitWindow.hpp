#pragma once
#include <horizon/Window.hpp>
#include <horizon/video/VideoView.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/Statusbar.hpp>
#include <horizon/ToolbarButton.hpp>

namespace zenit {

class ZenitWindow : public horizon::Window {
public:
    ZenitWindow();
    virtual ~ZenitWindow();

    void open_file(const std::string& path);
    
    uint32_t file_capabilities() const override;
    std::string current_file_path() const override;
    void set_application_recursive(horizon::WaylandWindow* app) override;
    horizon::CornerRadius get_window_corners() const override;

protected:
    void calculate_layout() override;

private:
    void setup_ui();
    void setup_context_menu();
    void update_controls();

    horizon::video::VideoView* m_video_view{nullptr};
    
    // UI Controls
    horizon::Statusbar* m_control_bar{nullptr};
    horizon::ToolbarButton* m_play_pause_btn{nullptr};
    horizon::ToolbarButton* m_rewind_btn{nullptr};
    horizon::ToolbarButton* m_forward_btn{nullptr};
    horizon::Label* m_current_time_label{nullptr};
    horizon::Label* m_total_time_label{nullptr};
    horizon::ProgressBar* m_progress_bar{nullptr};
    
    bool m_controls_visible{true};
};

} // namespace zenit
