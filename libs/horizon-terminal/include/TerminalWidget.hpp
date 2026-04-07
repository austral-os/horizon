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

namespace horizon {
namespace terminal {

class TerminalWidget : public horizon::Widget {
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
    
    // Scrollbar visuals
    std::unique_ptr<horizon::AquaPolygon> m_v_thumb;

    // Font resources
    FT_Library m_ft_library = nullptr;
    FT_Face m_ft_face = nullptr;
    hb_font_t* m_hb_font = nullptr;
    cairo_font_face_t* m_cairo_font_face = nullptr;
};

} // namespace terminal
} // namespace horizon
