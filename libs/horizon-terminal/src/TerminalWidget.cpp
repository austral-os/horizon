#include "TerminalWidget.hpp"
#include "horizon/GraphicsContext.hpp"
#include "horizon/WaylandWindow.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Clipboard.hpp"
#include "horizon/Menu.hpp"
#include "horizon/MenuItem.hpp"
#include "horizon/I18n.hpp"
#include <linux/input-event-codes.h>
#include <horizon/FileWatcher.hpp>


namespace horizon {
namespace terminal {

TerminalWidget::TerminalWidget() {
    set_focusable(true);
    set_background_color(Color(0.0f, 0.0f, 0.0f, 0.0f));
    
    m_config = ConfigReader::load();
    m_color_scheme = m_config.theme;
    init_fonts();
    start_watching(ConfigReader::get_config_path());

    m_v_thumb = std::make_unique<horizon::AquaPolygon>();
    m_v_thumb->set_accent_color(horizon::WidgetAccentColor::Primary);
    m_v_thumb->set_has_border(true);
    m_v_thumb->set_border_size(1.0f);
    m_v_thumb->set_rotation(270);

    m_controller = std::make_unique<TerminalController>(24, 80);
    m_controller->set_color_scheme(m_color_scheme);
    m_controller->set_scrollback_limit(m_config.scrollback_lines);
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

    when_mouse_wheel.connect([this](MouseWheelEventContext &ctx) {
        this->handle_mouse_wheel(ctx);
    });

    when_mouse_press.connect([this](MouseButtonEventContext &ctx) {
        this->handle_mouse_press(ctx);
    });

    when_mouse_drag.connect([this](MouseMoveEventContext &ctx) {
        this->handle_mouse_drag(ctx);
    });

    when_mouse_release.connect([this](MouseButtonEventContext &ctx) {
        this->handle_mouse_release(ctx);
    });

    when_right_click.connect([this](MouseButtonEventContext &ctx) {
        if (m_app) {
            auto menu = std::make_unique<horizon::Menu>();
            
            auto* copy_item = menu->add_item(i18n().tr("terminal.menu.copy"));
            copy_item->when_click.connect([this](auto&){ this->perform(horizon::ClipboardAction::Copy); });
            
            auto* paste_item = menu->add_item(i18n().tr("terminal.menu.paste"));
            paste_item->when_click.connect([this](auto&){ this->perform(horizon::ClipboardAction::Paste); });

            m_app->show_context_menu(menu.release(), -1, -1, ctx.serial, this);
            ctx.stop_propagation = true;
        }
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
    FcPatternAddInteger(pat, FC_WEIGHT, m_config.font_weight == 1 ? FC_WEIGHT_BOLD : FC_WEIGHT_NORMAL);
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
        // Use a standard character to determine monospaced width
        // and avoid max_advance which can be larger than the actual cell width
        if (FT_Load_Char(m_ft_face, 'M', FT_LOAD_DEFAULT) == 0) {
            m_char_width = m_ft_face->glyph->advance.x / 64.0;
        } else {
            m_char_width = m_ft_face->size->metrics.max_advance / 64.0;
        }
        m_char_height = m_ft_face->size->metrics.height / 64.0;
    } else {
        m_char_width = 8;
        m_char_height = 16;
    }

    if (m_char_width <= 0 || m_char_height <= 0) return;

    int available_width = width() - (m_config.show_scrollbar ? 14 : 0);
    int new_cols = available_width / m_char_width;
    int new_rows = height() / m_char_height;

    if (new_cols <= 0 || new_rows <= 0) return;

    if (new_cols != m_cols || new_rows != m_rows) {
        m_cols = new_cols;
        m_rows = new_rows;
        m_controller->resize(m_rows, m_cols);
        m_pty->resize(m_cols, m_rows);
        
        // Invalida la selección al redimensionar para evitar inconsistencias
        m_sel_start = m_sel_end = m_normalized_start = m_normalized_end = {-1, -1};
        m_is_selecting = false;
    }


    
    if (!m_initialized && m_cols > 0 && m_rows > 0) {
        m_initialized = true;
        spawn();
    }
}

VTermScreenCell TerminalWidget::get_cell_at(int r, int c, int size, int offset) {
    VTermScreenCell cell;
    memset(&cell, 0, sizeof(VTermScreenCell));
    
    // Fallback constants
    const char* fallback_bg = "#282a36"; // Dracula background
    const char* fallback_fg = "#f8f8f2"; // Dracula foreground

    int history_index = size - offset + r;
    if (history_index < size && history_index >= 0) {
        const auto& line = m_controller->get_scrollback_line(history_index);
        if (c < (int)line.cells.size()) {
            cell = line.cells[c];
        } else {
            cell.bg.type = VTERM_COLOR_DEFAULT_BG;
            cell.fg.type = VTERM_COLOR_DEFAULT_FG;
        }
    } else if (history_index >= size) {
        if (vterm_screen_get_cell(m_controller->get_screen(), {history_index - size, c}, &cell) == 0) {
            cell.bg.type = VTERM_COLOR_DEFAULT_BG;
            cell.fg.type = VTERM_COLOR_DEFAULT_FG;
        }
    } else {
        cell.bg.type = VTERM_COLOR_DEFAULT_BG;
        cell.fg.type = VTERM_COLOR_DEFAULT_FG;
    }

    // Capture "default" status BEFORE conversion
    bool is_default_bg = VTERM_COLOR_IS_DEFAULT_BG(&cell.bg);
    bool is_default_fg = VTERM_COLOR_IS_DEFAULT_FG(&cell.fg);

    // Convert potential palette/index colors to RGB
    VTermScreen* screen = m_controller->get_screen();
    vterm_screen_convert_color_to_rgb(screen, &cell.fg);
    vterm_screen_convert_color_to_rgb(screen, &cell.bg);

    // If it was default, FORCE our scheme colors
    if (is_default_bg) {
        horizon::Color c_bg(m_color_scheme.primary.background.empty() ? fallback_bg : m_color_scheme.primary.background);
        cell.bg.rgb.red = (uint8_t)(c_bg.r * 255);
        cell.bg.rgb.green = (uint8_t)(c_bg.g * 255);
        cell.bg.rgb.blue = (uint8_t)(c_bg.b * 255);
    }
    
    if (is_default_fg) {
        horizon::Color c_fg(m_color_scheme.primary.foreground.empty() ? fallback_fg : m_color_scheme.primary.foreground);
        cell.fg.rgb.red = (uint8_t)(c_fg.r * 255);
        cell.fg.rgb.green = (uint8_t)(c_fg.g * 255);
        cell.fg.rgb.blue = (uint8_t)(c_fg.b * 255);
    }

    if (cell.attrs.reverse) {
        VTermColor temp = cell.fg;
        cell.fg = cell.bg;
        cell.bg = temp;
    }

    return cell;
}

void TerminalWidget::draw(GraphicsContext &ctx) {
    cairo_t* cr = (cairo_t*)ctx.getNativeContext();
    if (!cr || !m_cairo_font_face) return;

    cairo_save(cr);

    // 1. Fill background with Cairo
    // Explicitly fallback to Dracula if scheme or background_color are not ready
    horizon::Color bg_color = m_color_scheme.primary.background.empty() ? background_color() : horizon::Color(m_color_scheme.primary.background);
    if (bg_color.to_hex() == "#000000" || bg_color.to_hex() == "#0d0d0d") {
        bg_color = horizon::Color("#282a36");
    }

    float alpha = m_config.transparency / 100.0f;
    cairo_set_source_rgba(cr, bg_color.r, bg_color.g, bg_color.b, alpha);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, x(), y(), width(), height());
    cairo_fill(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_set_font_face(cr, m_cairo_font_face);
    cairo_set_font_size(cr, m_config.font_size);

    int size = m_controller->get_scrollback_size();
    if (m_scroll_offset > size) m_scroll_offset = size;
    VTermScreen* screen = m_controller->get_screen();


    hb_buffer_t* hb_buf = hb_buffer_create();
    
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ) {
            VTermScreenCell cell = get_cell_at(r, c, size, m_scroll_offset);

            // Find segment of same style
            int start_c = c;
            std::vector<uint32_t> utf32_chars;
            utf32_chars.push_back(cell.chars[0] == 0 ? ' ' : cell.chars[0]);
            c++;
            
            while (c < m_cols) {
                VTermScreenCell next_cell = get_cell_at(r, c, size, m_scroll_offset);
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
                cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
                cairo_set_source_rgba(cr, cell.bg.rgb.red / 255.0, cell.bg.rgb.green / 255.0, cell.bg.rgb.blue / 255.0, alpha);
                cairo_rectangle(cr, x_pos, y_pos, (c - start_c) * m_char_width, m_char_height);
                cairo_fill(cr);
                cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
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
                    // Force the grid: advance exactly m_char_width per character cell
                    current_x += m_char_width;
                }

                // Set text color using converted RGB
                cairo_set_source_rgb(cr, cell.fg.rgb.red / 255.0, cell.fg.rgb.green / 255.0, cell.fg.rgb.blue / 255.0);

                cairo_show_glyphs(cr, cairo_glyphs.data(), glyph_count);
            }
        }
    }

    hb_buffer_destroy(hb_buf);

    if (m_cursor_visible && has_focus()) {
        double cursor_x = x() + m_cursor_pos.col * m_char_width;
        double cursor_visual_row = m_cursor_pos.row + m_scroll_offset;
        
        if (cursor_visual_row >= 0 && cursor_visual_row < m_rows) {
            double cursor_y = y() + cursor_visual_row * m_char_height;
            
            if (m_config.cursor_style == "bar") {
                horizon::Color cc(m_color_scheme.primary.cursor.empty() ? m_color_scheme.primary.foreground : m_color_scheme.primary.cursor);
                cairo_set_source_rgba(cr, cc.r, cc.g, cc.b, 1.0);
                cairo_rectangle(cr, cursor_x, cursor_y, 2.0, m_char_height);
                cairo_fill(cr);
            } else if (m_config.cursor_style == "underline") {
                horizon::Color cc(m_color_scheme.primary.cursor.empty() ? m_color_scheme.primary.foreground : m_color_scheme.primary.cursor);
                cairo_set_source_rgba(cr, cc.r, cc.g, cc.b, 1.0);
                // Draw a 2-pixel tall line at the bottom of the character's bounding box
                cairo_rectangle(cr, cursor_x, cursor_y + m_char_height - 2.0, m_char_width, 2.0);
                cairo_fill(cr);
            } else {
                horizon::Color cc(m_color_scheme.primary.cursor.empty() ? m_color_scheme.primary.foreground : m_color_scheme.primary.cursor);
                cairo_set_source_rgba(cr, cc.r, cc.g, cc.b, 0.5);
                cairo_rectangle(cr, cursor_x, cursor_y, m_char_width, m_char_height);
                cairo_fill(cr);
            }
        }
    }
        // --- Render Selection (Aesthetic Highlighting) ---
    if (m_normalized_start.row != -1) {
        int viewport_start_row = (int)m_controller->get_scrollback_size() - m_scroll_offset;
        
        cairo_set_source_rgba(cr, 0.2, 0.4, 0.8, 0.35); // Elegant Royal Blue
        
        for (int r = m_normalized_start.row; r <= m_normalized_end.row; ++r) {
            int widget_row = r - viewport_start_row;
            if (widget_row < 0 || widget_row >= m_rows) continue;
            
            int c_start = (r == m_normalized_start.row) ? m_normalized_start.col : 0;
            int c_end = (r == m_normalized_end.row) ? m_normalized_end.col : m_cols - 1;
            
            c_start = std::clamp(c_start, 0, m_cols - 1);
            c_end = std::clamp(c_end, 0, m_cols - 1);
            
            if (c_start <= c_end) {
                double rx = x() + c_start * m_char_width;
                double ry = y() + widget_row * m_char_height;
                double rw = (c_end - c_start + 1) * m_char_width;
                double rh = m_char_height;
                
                cairo_rectangle(cr, rx, ry, rw, rh);
                cairo_fill(cr);
            }
        }
    }

    cairo_restore(cr);
    
    if (m_config.show_scrollbar && size > 0) {

        int track_w = 12;
        int track_x = x() + width() - track_w - 2;
        int track_y = y() + 2;
        int track_h = height() - 4;

        ctx.setColor(horizon::Color(0.85f, 0.85f, 0.85f, 0.5f));
        ctx.fillRect(track_x, track_y, track_w, track_h, horizon::CornerRadius(track_w / 2.0));

        if (size > 0) {
            int total_lines = size + m_rows;
            double view_ratio = (double)m_rows / total_lines;
            int thumb_h = std::max(20, (int)(track_h * view_ratio));

            double max_y = track_h - thumb_h;
            int thumb_y = track_y + max_y * (1.0 - ((double)m_scroll_offset / size));

            std::vector<horizon::PolygonPoint> pts;
            int r = track_w / 2;
            pts.push_back({track_x, thumb_y, r});
            pts.push_back({track_x + track_w, thumb_y, r});
            pts.push_back({track_x + track_w, thumb_y + thumb_h, r});
            pts.push_back({track_x, thumb_y + thumb_h, r});
            m_v_thumb->set_points(pts);
        } else {
            std::vector<horizon::PolygonPoint> pts;
            int r = track_w / 2;
            pts.push_back({track_x, track_y, r});
            pts.push_back({track_x + track_w, track_y, r});
            pts.push_back({track_x + track_w, track_y + track_h, r});
            pts.push_back({track_x, track_y + track_h, r});
            m_v_thumb->set_points(pts);
        }
        
        if (!m_v_thumb->application() && application()) {
            m_v_thumb->set_application_recursive(application());
        }
        
        m_v_thumb->draw(ctx);
    }
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
            case KEY_LEFT: m_pty->write("\x1b[D", 3); break;
            case KEY_RIGHT: m_pty->write("\x1b[C", 3); break;
            case KEY_HOME: m_pty->write("\x1b[H", 3); break;
            case KEY_END: m_pty->write("\x1b[F", 3); break;
            case KEY_DELETE: m_pty->write("\x1b[3~", 4); break;
        }
    }
}



void TerminalWidget::on_pty_read(const char* data, size_t len) {
    m_controller->push_data(data, len);
    if (m_scroll_offset > 0) {
        m_scroll_offset = 0;
        invalidate();
    }
}



void TerminalWidget::handle_mouse_wheel(MouseWheelEventContext &ctx) {
    if (!m_config.show_scrollbar && !m_config.scroll_without_scrollbar) return;
    
    int size = m_controller->get_scrollback_size();
    if (size == 0) return;
    
    if (ctx.dy < 0) {
        m_scroll_offset += 3;
    } else if (ctx.dy > 0) {
        m_scroll_offset -= 3;
    }

    if (m_scroll_offset > size) m_scroll_offset = size;
    if (m_scroll_offset < 0) m_scroll_offset = 0;
    
    invalidate();
}

void TerminalWidget::handle_mouse_press(MouseButtonEventContext &ctx) {
    set_focus(true);
    
    int size = m_controller->get_scrollback_size();
    if (m_config.show_scrollbar && size > 0) {

        int track_w = 12;
        int track_x = x() + width() - track_w - 2;
        int track_y = y() + 2;
        int track_h = height() - 4;
        
        if (ctx.x >= track_x && ctx.x <= track_x + track_w &&
            ctx.y >= track_y && ctx.y <= track_y + track_h) {
            
            m_dragging_scrollbar = true;
            m_drag_start_y = (int)ctx.y;
            m_drag_start_offset = m_scroll_offset;
            ctx.stop_propagation = true;
            return;
        }
    }

    // Comienzo de selección (preparación)
    if (ctx.button == 0x110) { // BTN_LEFT
        m_sel_start = screen_to_buffer(ctx.x, ctx.y);
        m_sel_end = m_sel_start;
        
        // No mostrar selección hasta que haya arrastre
        m_normalized_start = {-1, -1};
        m_normalized_end = {-1, -1};
        
        m_is_selecting = true;
        invalidate();
    }
}








void TerminalWidget::handle_mouse_drag(MouseMoveEventContext &ctx) {
    if (m_dragging_scrollbar) {
        int size = (int)m_controller->get_scrollback_size();
        if (size > 0) {
            int track_h = height() - 4;
            int total_lines = size + m_rows;
            double view_ratio = (double)m_rows / total_lines;
            int thumb_h = std::max(20, (int)(track_h * view_ratio));
            int max_y = track_h - thumb_h;
            
            if (max_y > 0) {
                int delta_y = (int)ctx.y - m_drag_start_y;
                int offset_delta = (int)(delta_y * ((double)size / max_y));
                m_scroll_offset = m_drag_start_offset - offset_delta;
                
                if (m_scroll_offset > size) m_scroll_offset = size;
                if (m_scroll_offset < 0) m_scroll_offset = 0;
                
                invalidate();
            }
        }
        ctx.stop_propagation = true;
    } else if (m_is_selecting) {
        update_selection(screen_to_buffer(ctx.x, ctx.y));
        invalidate();
    }


}



void TerminalWidget::handle_mouse_release(MouseButtonEventContext &ctx) {
    if (m_dragging_scrollbar) {
        m_dragging_scrollbar = false;
    } else if (m_is_selecting) {
        m_is_selecting = false;
        if (!(m_sel_start.row == m_sel_end.row && m_sel_start.col == m_sel_end.col)) {
            copy_selection();
        }
        invalidate();
    }
}


void TerminalWidget::on_terminal_damage(VTermRect rect) {
    if (m_app) {
        m_app->post_task([this]() {
            this->invalidate();
        });
    }
}

BufferPos TerminalWidget::screen_to_buffer(double x, double y) {
    if (m_char_width == 0 || m_char_height == 0) return {0, 0};
    
    int col = (int)((x - this->x()) / m_char_width);
    int row = (int)((y - this->y()) / m_char_height);
    
    int sb_size = (int)m_controller->get_scrollback_size();
    int total_rows = m_controller->get_total_rows();
    
    // El viewport comienza en sb_size - m_scroll_offset
    int abs_row = sb_size - m_scroll_offset + row;
    
    // Clamping robusto
    abs_row = std::clamp(abs_row, 0, total_rows - 1);
    col = std::clamp(col, 0, m_cols - 1);
    
    return { abs_row, col };
}

void TerminalWidget::update_selection(BufferPos end_pos) {
    m_sel_end = end_pos;
    
    // Normalizar y cachear
    if (m_sel_start.row < m_sel_end.row || (m_sel_start.row == m_sel_end.row && m_sel_start.col <= m_sel_end.col)) {
        m_normalized_start = m_sel_start;
        m_normalized_end = m_sel_end;
    } else {
        m_normalized_start = m_sel_end;
        m_normalized_end = m_sel_start;
    }
    
    std::cerr << "[TERMINAL DEBUG] Selection Normalized: [" << m_normalized_start.row << "," << m_normalized_start.col << "] -> [" << m_normalized_end.row << "," << m_normalized_end.col << "]" << std::endl;
}



void TerminalWidget::copy_selection() {
    if (m_normalized_start.row == -1) return;
    
    std::string result;
    int cols = m_cols;
    int total_rows = m_controller->get_total_rows();
    
    // Reservar espacio aproximado para evitar reasignaciones
    result.reserve((m_normalized_end.row - m_normalized_start.row + 1) * cols);
    
    for (int r = m_normalized_start.row; r <= m_normalized_end.row; ++r) {
        if (r >= total_rows) break;

        int c_start = (r == m_normalized_start.row) ? m_normalized_start.col : 0;
        int c_end = (r == m_normalized_end.row) ? m_normalized_end.col : cols - 1;
        
        // Clamp por fila
        c_start = std::clamp(c_start, 0, cols - 1);
        c_end = std::clamp(c_end, 0, cols - 1);
        
        std::string line_text;
        for (int c = c_start; c <= c_end; ++c) {
            auto cell = m_controller->get_cell(r, c);
            if (!cell.is_continuation) {
                line_text += cell.text;
            }
        }
        
        // Recortar espacios finales solo si llegamos al final de la línea lógica/visual
        if (c_end == cols - 1) {
            size_t last = line_text.find_last_not_of(' ');
            if (last != std::string::npos) {
                line_text.erase(last + 1);
            } else if (!line_text.empty() && line_text[0] == ' ') {
                // Línea de puros espacios
                line_text.clear();
            }
        }
        
        result += line_text;
        
        // Salto de línea lógico: solo si no está envuelta (wrapped) la SIGUIENTE línea
        if (r < m_normalized_end.row && r < total_rows - 1) {
             auto next_cell = m_controller->get_cell(r + 1, 0);
             if (!next_cell.wrapped) {
                 result += "\n";
             }
        }
    }
    
    if (!result.empty()) {
        m_clipboard_content = result;
        if (m_app) {
            m_app->set_clipboard_owner(this);
        }
    }
}

void TerminalWidget::set_application_recursive(WaylandWindow *app) {

    Widget::set_application_recursive(app);
    if (m_app && m_cursor_timer == 0) {
        m_cursor_timer = m_app->add_timer(600, [this]() {
            if (this->has_focus()) {
                if (m_config.cursor_blink) {
                    this->m_cursor_visible = !this->m_cursor_visible;
                } else {
                    this->m_cursor_visible = true;
                }
                this->invalidate();
            } else {
                this->m_cursor_visible = true; // when focus comes back, start visible
            }
        }, true);
    }
}

bool TerminalWidget::can_perform(horizon::ClipboardAction action) const {
    if (action == horizon::ClipboardAction::Copy || action == horizon::ClipboardAction::Cut) {
        return m_sel_start.row != -1;
    }
    if (action == horizon::ClipboardAction::Paste) {
        return true;
    }
    return false;
}

void TerminalWidget::perform(horizon::ClipboardAction action) {
    if (action == horizon::ClipboardAction::Copy || action == horizon::ClipboardAction::Cut) {
        copy_selection();
    } else if (action == horizon::ClipboardAction::Paste) {
        if (m_app) {
            auto mimes = m_app->get_clipboard_mime_types();
            std::string best_mime = "text/plain";
            for (const auto& mime : mimes) {
                if (mime == "text/uri-list") {
                    best_mime = mime;
                    break;
                }
            }
            m_app->request_clipboard_data(this, best_mime);
        }
    }
}

void TerminalWidget::write(const std::vector<uint8_t>& data) {
    if (m_pty && !data.empty()) {
        m_pty->write((const char*)data.data(), data.size());
    }
}

void TerminalWidget::provide_clipboard_data(const std::string& mime, horizon::DataSink& sink) {
    if (mime == "text/plain" || mime == "text/plain;charset=utf-8") {
        if (!m_clipboard_content.empty()) {
            std::vector<uint8_t> data_vec(m_clipboard_content.begin(), m_clipboard_content.end());
            sink.write(data_vec);
            sink.done();
        }
    } else {
        sink.error();
    }
}

std::vector<std::string> TerminalWidget::provided_mime_types() const {
    return {"text/plain", "text/plain;charset=utf-8"};
}

std::vector<std::string> TerminalWidget::accepted_mime_types() const {
    return {"text/plain", "text/uri-list"};
}

void TerminalWidget::on_clipboard_data_received(const std::string& mime, const std::vector<uint8_t>& data) {
    LOG_INFO << "TerminalWidget: on_clipboard_data_received starting. Mime: " << mime << ", Size: " << data.size();
    if (mime == "text/uri-list") {
        std::string content((const char*)data.data(), data.size());
        LOG_INFO << "TerminalWidget: Raw content: " << content;
        std::stringstream ss(content);
        std::string line;
        std::string result;
        
        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            // Remove \r and "file://" prefix
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.find("file://") == 0) line = line.substr(7);
            
            // Add space between items
            if (!result.empty()) result += " ";
            
            // Wrap in single quotes if it contains spaces or special characters
            if (line.find(' ') != std::string::npos || line.find('\'') != std::string::npos) {
                // escape single quotes
                std::string escaped;
                for (char c : line) {
                    if (c == '\'') escaped += "'\\''";
                    else escaped += c;
                }
                result += "'" + escaped + "'";
            } else {
                result += line;
            }
        }
        
        if (!result.empty()) {
            std::vector<uint8_t> result_vec(result.begin(), result.end());
            write(result_vec);
        }
    } else if (mime == "text/plain" || mime == "text/plain;charset=utf-8") {
        write(data);
    }
}

void TerminalWidget::on_file_changed() {
    LOG_INFO << "[TERMINAL] Config change detected, reloading...";
    reload_config();
}

void TerminalWidget::post_watcher_task(std::function<void()> task) {
    if (m_app) {
        m_app->post_task(task);
    }
}

void TerminalWidget::reload_config() {
    m_config = ConfigReader::load();
    set_color_scheme(m_config.theme);
    
    // Refresh fonts
    cleanup_fonts();
    init_fonts();

    // Update controller settings
    if (m_controller) {
        m_controller->set_scrollback_limit(m_config.scrollback_lines);
    }

    if (application()) {
        application()->set_blur(m_config.blur);
    }

    // Refresh layout and visuals
    calculate_layout();
    invalidate();
    
    LOG_INFO << "[TERMINAL] Configuration reloaded.";
}

void TerminalWidget::set_color_scheme(const TerminalColorScheme& scheme) {
    m_color_scheme = scheme;
    if (m_controller) {
        m_controller->set_color_scheme(scheme);
    }
    
    invalidate();
}

} // namespace terminal
} // namespace horizon
