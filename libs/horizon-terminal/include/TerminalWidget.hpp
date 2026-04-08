#pragma once

#include "horizon/Widget.hpp"
#include "TerminalController.hpp"
#include "PtyHandler.hpp"
#include "ConfigReader.hpp"
#include <memory>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <horizon/AquaPolygon.hpp>
#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>

#include <horizon/ClipboardProvider.hpp>

namespace horizon {
namespace terminal {

class TerminalWidget : public horizon::Widget, public horizon::DataSink {
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

    // Clipboard Support
    bool supports_clipboard() const override { return true; }
    bool can_perform(horizon::ClipboardAction action) const override;
    void perform(horizon::ClipboardAction action) override;

    // DataSink overrides
    void write(const std::vector<uint8_t>& data) override;
    void done() override {}
    void error() override {}

    // ClipboardProvider overrides
    void provide_clipboard_data(const std::string& mime, horizon::DataSink& sink) override;
    std::vector<std::string> provided_mime_types() const override;
    horizon::ClipboardProvider *get_clipboard_provider() override { return this; }


    void set_application_recursive(horizon::WaylandWindow *app) override;

private:
    void update_pty_size();
    void on_pty_read(const char* data, size_t len);
    void on_terminal_damage(VTermRect rect);
    
    bool init_fonts();
    void cleanup_fonts();

    std::unique_ptr<TerminalController> m_controller;
    std::unique_ptr<PtyHandler> m_pty;
    
    int m_rows = 0;
    int m_cols = 0;
    double m_char_width = 0;
    double m_char_height = 0;
    
    bool m_initialized = false;
    TerminalConfig m_config;

    // Cursor state
    bool m_cursor_visible = true;
    size_t m_cursor_timer = 0;
    VTermPos m_cursor_pos = {0, 0};
    
    // Scrollback
    int m_scroll_offset = 0;

    // Selection
    BufferPos m_sel_start = {-1, -1};
    BufferPos m_sel_end = {-1, -1};
    bool m_is_selecting = false;
    BufferPos m_normalized_start = {-1, -1};
    BufferPos m_normalized_end = {-1, -1};
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
