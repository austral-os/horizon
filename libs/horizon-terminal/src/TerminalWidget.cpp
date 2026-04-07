#include "TerminalWidget.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/WaylandWindow.hpp"
#include "horizon/Logger.hpp"
#include <linux/input-event-codes.h>
#include <iostream>

namespace horizon {
namespace terminal {

TerminalWidget::TerminalWidget() {
    set_focusable(true);
    set_background_color(Color(0.05f, 0.05f, 0.05f, 1.0f));
    
    m_config = ConfigReader::load();
    init_fonts();

    m_controller = std::make_unique<TerminalController>(24, 80);
    m_pty = std::make_unique<PtyHandler>();

    m_pty->set_read_callback([this](const char* data, size_t len) {
        this->on_pty_read(data, len);
    });

    m_controller->set_damage_callback([this](VTermRect rect) {
        this->on_terminal_damage(rect);
    });

    m_controller->set_move_cursor_callback([this](VTermPos pos) {
        this->m_cursor_pos = pos;
        this->invalidate();
    });

    when_key_press.connect([this](KeyEventContext &ctx) {
        this->handle_key_press(ctx);
    });

    when_mouse_press.connect([this](MouseButtonEventContext &ctx) {
        this->set_focus(true);
    });
}

TerminalWidget::~TerminalWidget() {
    if (m_pty) {
        m_pty->close();
    }
    cleanup_fonts();
}

bool TerminalWidget::init_fonts() {
    if (FT_Init_FreeType(&m_ft_library) != 0) return false;

    // Use Fontconfig to find the font path
    FcConfig* config = FcInitLoadConfigAndFonts();
    FcPattern* pat = FcNameParse((const FcChar8*)m_config.font.c_str());
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    
    FcResult result;
    FcPattern* match = FcFontMatch(config, pat, &result);
    bool found = false;
    if (match) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch) {
            if (FT_New_Face(m_ft_library, (const char*)file, 0, &m_ft_face) == 0) {
                found = true;
            }
        }
        FcPatternDestroy(match);
    }
    FcPatternDestroy(pat);
    FcConfigDestroy(config);

    if (!found) {
        // Fallback or retry with "Monospace"
        FT_New_Face(m_ft_library, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 0, &m_ft_face);
    }

    if (m_ft_face) {
        FT_Set_Pixel_Sizes(m_ft_face, 0, m_config.font_size);
        m_hb_font = hb_ft_font_create(m_ft_face, nullptr);
        m_cairo_font_face = cairo_ft_font_face_create_for_ft_face(m_ft_face, 0);
    }

    return m_ft_face != nullptr;
}

void TerminalWidget::cleanup_fonts() {
    if (m_cairo_font_face) cairo_font_face_destroy(m_cairo_font_face);
    if (m_hb_font) hb_font_destroy(m_hb_font);
    if (m_ft_face) FT_Done_Face(m_ft_face);
    if (m_ft_library) FT_Done_FreeType(m_ft_library);
}

void TerminalWidget::spawn(const std::string& shell) {
    if (m_cols > 0 && m_rows > 0) {
        m_pty->spawn(shell, {}, m_cols, m_rows);
    }
}

void TerminalWidget::calculate_layout() {
    Widget::calculate_layout();

    if (m_ft_face) {
        m_char_width = m_ft_face->size->metrics.max_advance / 64.0;
        m_char_height = m_ft_face->size->metrics.height / 64.0;
    } else {
        m_char_width = 8;
        m_char_height = 16;
    }

    int new_cols = width() / m_char_width;
    int new_rows = height() / m_char_height;

    if (new_cols != m_cols || new_rows != m_rows) {
        m_cols = new_cols;
        m_rows = new_rows;
        m_controller->resize(m_rows, m_cols);
        m_pty->resize(m_cols, m_rows);
    }
    
    if (!m_initialized && m_cols > 0 && m_rows > 0) {
        m_initialized = true;
        spawn();
    }
}

void TerminalWidget::draw(GraphicsContext &ctx) {
    // Fill background
    ctx.setColor(background_color());
    ctx.fillRect(x(), y(), width(), height());

    cairo_t* cr = (cairo_t*)ctx.getNativeContext();
    if (!cr || !m_cairo_font_face) return;

    cairo_save(cr);
    cairo_set_font_face(cr, m_cairo_font_face);
    cairo_set_font_size(cr, m_config.font_size);

    VTermScreen* screen = m_controller->get_screen();
    hb_buffer_t* hb_buf = hb_buffer_create();
    
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ) {
            VTermPos pos = {r, c};
            VTermScreenCell cell;
            if (!vterm_screen_get_cell(screen, pos, &cell)) {
                c++; continue;
            }

            // Find segment of same style
            int start_c = c;
            std::vector<uint32_t> utf32_chars;
            utf32_chars.push_back(cell.chars[0] == 0 ? ' ' : cell.chars[0]);
            c++;
            
            while (c < m_cols) {
                VTermScreenCell next_cell;
                vterm_screen_get_cell(screen, {r, c}, &next_cell);
                // Simple style compare for now (colors)
                if (next_cell.bg.rgb.red != cell.bg.rgb.red || 
                    next_cell.bg.rgb.green != cell.bg.rgb.green ||
                    next_cell.bg.rgb.blue != cell.bg.rgb.blue ||
                    next_cell.fg.rgb.red != cell.fg.rgb.red || 
                    next_cell.fg.rgb.green != cell.fg.rgb.green ||
                    next_cell.fg.rgb.blue != cell.fg.rgb.blue) {
                    break;
                }
                utf32_chars.push_back(next_cell.chars[0] == 0 ? ' ' : next_cell.chars[0]);
                c++;
            }

            double x_pos = x() + start_c * m_char_width;
            double y_pos = y() + r * m_char_height;

            // Draw segment background
            if (!VTERM_COLOR_IS_DEFAULT_BG(&cell.bg)) {
                vterm_screen_convert_color_to_rgb(screen, &cell.bg);
                cairo_set_source_rgb(cr, cell.bg.rgb.red / 255.0, cell.bg.rgb.green / 255.0, cell.bg.rgb.blue / 255.0);
                cairo_rectangle(cr, x_pos, y_pos, (c - start_c) * m_char_width, m_char_height);
                cairo_fill(cr);
            }

            // Draw text segment with HarfBuzz
            if (!utf32_chars.empty()) {
                hb_buffer_clear_contents(hb_buf);
                hb_buffer_add_utf32(hb_buf, utf32_chars.data(), utf32_chars.size(), 0, utf32_chars.size());
                hb_buffer_guess_segment_properties(hb_buf);
                hb_shape(m_hb_font, hb_buf, nullptr, 0);

                unsigned int glyph_count;
                hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
                hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hb_buf, &glyph_count);

                std::vector<cairo_glyph_t> cairo_glyphs(glyph_count);
                double current_x = x_pos;
                double current_y = y_pos + (m_ft_face->size->metrics.ascender / 64.0);

                for (unsigned int i = 0; i < glyph_count; ++i) {
                    cairo_glyphs[i].index = glyph_info[i].codepoint;
                    cairo_glyphs[i].x = current_x + (glyph_pos[i].x_offset / 64.0);
                    cairo_glyphs[i].y = current_y - (glyph_pos[i].y_offset / 64.0);
                    current_x += (glyph_pos[i].x_advance / 64.0);
                }

                if (!VTERM_COLOR_IS_DEFAULT_FG(&cell.fg)) {
                    vterm_screen_convert_color_to_rgb(screen, &cell.fg);
                    cairo_set_source_rgb(cr, cell.fg.rgb.red / 255.0, cell.fg.rgb.green / 255.0, cell.fg.rgb.blue / 255.0);
                } else {
                    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
                }

                cairo_show_glyphs(cr, cairo_glyphs.data(), glyph_count);
            }
        }
    }

    hb_buffer_destroy(hb_buf);

    if (m_cursor_visible && has_focus()) {
        double cursor_x = x() + m_cursor_pos.col * m_char_width;
        double cursor_y = y() + m_cursor_pos.row * m_char_height;
        
        if (m_config.cursor_style == "bar") {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
            cairo_rectangle(cr, cursor_x, cursor_y, 2.0, m_char_height);
            cairo_fill(cr);
        } else if (m_config.cursor_style == "underline") {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
            // Draw a 2-pixel tall line at the bottom of the character's bounding box
            cairo_rectangle(cr, cursor_x, cursor_y + m_char_height - 2.0, m_char_width, 2.0);
            cairo_fill(cr);
        } else {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.5);
            cairo_rectangle(cr, cursor_x, cursor_y, m_char_width, m_char_height);
            cairo_fill(cr);
        }
    }

    cairo_restore(cr);
}

void TerminalWidget::handle_key_press(KeyEventContext &ctx) {
    if (ctx.text.length() > 0) {
        m_pty->write(ctx.text.c_str(), ctx.text.length());
    } else {
        switch (ctx.key) {
            case KEY_ENTER: m_pty->write("\r", 1); break;
            case KEY_BACKSPACE: m_pty->write("\x7f", 1); break;
            case KEY_TAB: m_pty->write("\t", 1); break;
            case KEY_ESC: m_pty->write("\x1b", 1); break;
            case KEY_UP: m_pty->write("\x1b[A", 3); break;
            case KEY_DOWN: m_pty->write("\x1b[B", 3); break;
            case KEY_RIGHT: m_pty->write("\x1b[C", 3); break;
            case KEY_LEFT: m_pty->write("\x1b[D", 3); break;
        }
    }
}

void TerminalWidget::on_pty_read(const char* data, size_t len) {
    m_controller->push_data(data, len);
}

void TerminalWidget::on_terminal_damage(VTermRect rect) {
    if (m_app) {
        m_app->post_task([this]() {
            this->invalidate();
        });
    }
}

void TerminalWidget::set_application_recursive(WaylandWindow *app) {
    Widget::set_application_recursive(app);
    if (m_app && m_cursor_timer == 0) {
        m_cursor_timer = m_app->add_timer(600, [this]() {
            if (this->has_focus()) {
                this->m_cursor_visible = !this->m_cursor_visible;
                this->invalidate();
            } else {
                this->m_cursor_visible = true; // when focus comes back, start visible
            }
        }, true);
    }
}

} // namespace terminal
} // namespace horizon
