#pragma once

#include "horizon/Widget.hpp"
#include "TerminalController.hpp"
#include "ConfigReader.hpp"
#include "TerminalColorScheme.hpp"
#include <horizon/FileWatcher.hpp>
#include <memory>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <horizon/AquaPolygon.hpp>
#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

#include <horizon/ClipboardProvider.hpp>

namespace horizon {
namespace terminal {

class TerminalWidget : public horizon::Widget, public horizon::DataSink, public horizon::FileWatcher {
public:
    TerminalWidget();
    ~TerminalWidget();

    void spawn(const std::string& shell = "/bin/bash");
    
    // Core widget overrides
    void draw(horizon::GraphicsContext &ctx) override;
    void calculate_layout() override;

    // Event handlers
    void handle_key_press(horizon::KeyEventContext &ctx);
    void handle_mouse_wheel(horizon::MouseWheelEventContext &ctx);
    void handle_mouse_press(horizon::MouseButtonEventContext &ctx);
    void handle_mouse_drag(horizon::MouseMoveEventContext &ctx);
    void handle_mouse_release(horizon::MouseButtonEventContext &ctx);

    BufferPos screen_to_buffer(double x, double y);
    void update_selection(BufferPos end_pos);
    void copy_selection();

    // Fullscreen Support
    bool supports_fullscreen() const override { return true; }

    // Clipboard Support
    bool supports_clipboard() const override { return true; }
    bool can_perform(horizon::ClipboardAction action) const override;
    void perform(horizon::ClipboardAction action) override;
    std::vector<std::string> accepted_mime_types() const override;
    void on_clipboard_data_received(const std::string& mime, const std::vector<uint8_t>& data) override;

    // DataSink overrides
    void write(const std::vector<uint8_t>& data) override;
    void done() override {}
    void error() override {}

    // ClipboardProvider overrides
    void provide_clipboard_data(const std::string& mime, horizon::DataSink& sink) override;
    std::vector<std::string> provided_mime_types() const override;
    horizon::ClipboardProvider *get_clipboard_provider() override { return this; }


    void set_application_recursive(horizon::WaylandWindow *app) override;

    void set_color_scheme(const TerminalColorScheme& scheme);

private:
    void update_pty_size();
    void on_pty_read(const char* data, size_t len);
    void flush_pending_pty_data();
    void on_terminal_damage(VTermRect rect);
    VTermScreenCell get_cell_at(int r, int c, int size, int offset);
    
    bool init_fonts();
    void cleanup_fonts();
    void reload_config();

    // FileWatcher overrides
    void on_file_changed() override;
    void post_watcher_task(std::function<void()> task) override;

    std::unique_ptr<TerminalController> m_controller;
    std::unique_ptr<PtyHandler> m_pty;
    std::shared_ptr<std::atomic<bool>> m_alive{std::make_shared<std::atomic<bool>>(true)};
    std::mutex m_pending_pty_mutex;
    std::vector<uint8_t> m_pending_pty_data;
    bool m_pty_flush_pending{false};
    
    int m_rows = 0;
    int m_cols = 0;
    double m_char_width = 0;
    double m_char_height = 0;
    
    bool m_initialized = false;
    TerminalConfig m_config;
    TerminalColorScheme m_color_scheme;

    // Cursor state
    bool m_cursor_visible = true;
    size_t m_cursor_timer = 0;
    size_t m_resize_timer = 0;
    VTermPos m_cursor_pos = {0, 0};
    uint32_t m_last_modifiers = 0;
    
    // Scrollback
    int m_scroll_offset = 0;

    // Selection
    BufferPos m_sel_start = {-1, -1};
    BufferPos m_sel_end = {-1, -1};
    bool m_is_selecting = false;
    BufferPos m_normalized_start = {-1, -1};
    BufferPos m_normalized_end = {-1, -1};
    bool m_keyboard_selecting = false;
    std::string m_clipboard_content;

    // Scrollbar visuals

    std::unique_ptr<horizon::AquaPolygon> m_v_thumb;
    bool m_dragging_scrollbar = false;
    int m_drag_start_y = 0;
    int m_drag_start_offset = 0;

    // Font resources
    FT_Library m_ft_library = nullptr;
    FT_Face m_ft_face = nullptr;
    hb_font_t* m_hb_font = nullptr;
    cairo_font_face_t* m_cairo_font_face = nullptr;
};

} // namespace terminal
} // namespace horizon
