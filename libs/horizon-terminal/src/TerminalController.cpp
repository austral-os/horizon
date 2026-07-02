#include "TerminalController.hpp"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <horizon/Color.hpp>


static std::string utf32_to_utf8(uint32_t codepoint) {
    if (codepoint == 0) return "";
    std::string res;
    if (codepoint < 0x80) {
        res += (char)codepoint;
    } else if (codepoint < 0x800) {
        res += (char)(0xC0 | (codepoint >> 6));
        res += (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        res += (char)(0xE0 | (codepoint >> 12));
        res += (char)(0x80 | ((codepoint >> 6) & 0x3F));
        res += (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x110000) {
        res += (char)(0xF0 | (codepoint >> 18));
        res += (char)(0x80 | ((codepoint >> 12) & 0x3F));
        res += (char)(0x80 | ((codepoint >> 6) & 0x3F));
        res += (char)(0x80 | (codepoint & 0x3F));
    }
    return res;
}


namespace horizon {
namespace terminal {

TerminalController::TerminalController(int rows, int cols) {
    m_vt = vterm_new(rows, cols);
    vterm_set_utf8(m_vt, 1);
    m_screen = vterm_obtain_screen(m_vt);
    vterm_screen_enable_altscreen(m_screen, 1);
    vterm_screen_set_reflow(m_screen, 1);
    
    // Set default colors (matches TerminalWidget theme)
    VTermState* state = vterm_obtain_state(m_vt);
    VTermColor fg_color, bg_color;
    fg_color.type = VTERM_COLOR_RGB;
    fg_color.rgb.red = 230; fg_color.rgb.green = 230; fg_color.rgb.blue = 230;
    bg_color.type = VTERM_COLOR_RGB;
    bg_color.rgb.red = 13; bg_color.rgb.green = 13; bg_color.rgb.blue = 13;
    vterm_state_set_default_colors(state, &fg_color, &bg_color);

    vterm_screen_reset(m_screen, 1);

    static VTermScreenCallbacks screen_callbacks = {
        .damage = screen_damage,
        .moverect = screen_moverect,
        .movecursor = screen_movecursor,
        .settermprop = screen_settermprop,
        .bell = screen_bell,
        .resize = screen_resize,
        .sb_pushline = screen_sb_pushline,
        .sb_popline = screen_sb_popline,
        .sb_clear = screen_sb_clear
    };

    vterm_screen_set_callbacks(m_screen, &screen_callbacks, this);
}

TerminalController::~TerminalController() {
    if (m_vt) {
        vterm_free(m_vt);
    }
}

void TerminalController::push_data(const char* data, size_t len) {
    std::lock_guard<std::mutex> lock(m_mutex);
    vterm_input_write(m_vt, data, len);
    vterm_screen_flush_damage(m_screen);
}

void TerminalController::write_to_pty(const char* data, size_t len) {
    // This is handled by the widget/pty connection
}

void TerminalController::resize(int rows, int cols) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_is_resizing = true;
    vterm_set_size(m_vt, rows, cols);
    m_is_resizing = false;
}

// Callbacks
int TerminalController::screen_damage(VTermRect rect, void *user) {
    auto self = static_cast<TerminalController*>(user);
    if (self->m_damage_cb) {
        self->m_damage_cb(rect);
    }
    return 1;
}

int TerminalController::screen_moverect(VTermRect dest, VTermRect src, void *user) {
    auto self = static_cast<TerminalController*>(user);
    if (self->m_damage_cb) {
        int rows, cols;
        vterm_get_size(self->m_vt, &rows, &cols);
        VTermRect full_rect = {0, rows, 0, cols};
        self->m_damage_cb(full_rect);
    }
    return 1;
}

int TerminalController::screen_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user) {
    auto self = static_cast<TerminalController*>(user);
    if (self->m_move_cursor_cb) {
        self->m_move_cursor_cb(pos);
    }
    return 1;
}

int TerminalController::screen_settermprop(VTermProp prop, VTermValue *val, void *user) {
    auto self = static_cast<TerminalController*>(user);
    if (prop == VTERM_PROP_ALTSCREEN) {
        self->m_is_altscreen = val->boolean;
        // Invalidate full screen when switching buffers
        if (self->m_damage_cb) {
            int rows, cols;
            vterm_get_size(self->m_vt, &rows, &cols);
            VTermRect rect = {0, rows, 0, cols};
            self->m_damage_cb(rect);
        }
    } else if (prop == VTERM_PROP_MOUSE) {
        self->m_mouse_mode = val->number;
    }
    return 1;
}

int TerminalController::screen_bell(void *user) {
    return 1;
}

int TerminalController::screen_resize(int rows, int cols, void *user) {
    return 1;
}

int TerminalController::screen_sb_pushline(int cols, const VTermScreenCell *cells, void *user) {
    auto self = static_cast<TerminalController*>(user);
    
    // Do not push to scrollback if we are in the alternative screen
    if (self->m_is_altscreen) {
        return 1;
    }

    // El scrollback de libvterm empuja la línea 0 (la de arriba) cuando hay scroll hacia abajo.
    VTermState* state = vterm_obtain_state(self->m_vt);
    const VTermLineInfo* info = vterm_state_get_lineinfo(state, 0);

    ScrollbackLine line;
    line.cells.assign(cells, cells + cols);
    line.wrapped = info ? info->continuation : false;

    self->m_scrollback_buffer.push_front(line);
    
    while (self->m_scrollback_buffer.size() > self->m_scrollback_limit) {
        self->m_scrollback_buffer.pop_back();
    }
    return 1;
}


int TerminalController::screen_sb_popline(int cols, VTermScreenCell *cells, void *user) {
    auto self = static_cast<TerminalController*>(user);
    
    // Initialize the buffer to avoid artifacts from uninitialized memory
    for (int i = 0; i < cols; ++i) {
        memset(&cells[i], 0, sizeof(VTermScreenCell));
        cells[i].chars[0] = 0;
        cells[i].width = 1;
        cells[i].bg.type = VTERM_COLOR_DEFAULT_BG;
        cells[i].fg.type = VTERM_COLOR_DEFAULT_FG;
    }

    if (self->m_scrollback_buffer.empty()) {
        return 0;
    }
    
    auto& line = self->m_scrollback_buffer.front();
    int copy_cols = std::min(cols, (int)line.cells.size());
    
    // Copy existing cells over the clean buffer
    std::copy(line.cells.begin(), line.cells.begin() + copy_cols, cells);
    
    self->m_scrollback_buffer.pop_front();
    return 1;
}


int TerminalController::screen_sb_clear(void *user) {
    auto self = static_cast<TerminalController*>(user);
        self->m_scrollback_buffer.clear();
    return 1;
}

void TerminalController::set_color_scheme(const TerminalColorScheme& scheme) {
    VTermState* state = vterm_obtain_state(m_vt);
    
    auto to_vterm_color = [](const std::string& hex) {
        horizon::Color c(hex);
        VTermColor vc;
        vc.type = VTERM_COLOR_RGB;
        vc.rgb.red = (uint8_t)(c.r * 255);
        vc.rgb.green = (uint8_t)(c.g * 255);
        vc.rgb.blue = (uint8_t)(c.b * 255);
        return vc;
    };

    VTermColor fg = to_vterm_color(scheme.primary.foreground);
    VTermColor bg = to_vterm_color(scheme.primary.background);
    vterm_state_set_default_colors(state, &fg, &bg);

    // Normal colors (0-7)
    std::vector<std::string> normal_colors = scheme.normal.to_vector();
    for (int i = 0; i < 8; ++i) {
        VTermColor vc = to_vterm_color(normal_colors[i]);
        vterm_state_set_palette_color(state, i, &vc);
    }

    // Bright colors (8-15)
    std::vector<std::string> bright_colors = scheme.bright.to_vector();
    for (int i = 0; i < 8; ++i) {
        VTermColor vc = to_vterm_color(bright_colors[i]);
        vterm_state_set_palette_color(state, i + 8, &vc);
    }
}

VTermScreenCell TerminalController::get_vterm_cell(int row, int col) {
    auto clear_cell = [](VTermScreenCell &cell) {
        memset(&cell, 0, sizeof(cell));
        cell.width = 1;
        cell.bg.type = VTERM_COLOR_DEFAULT_BG;
        cell.fg.type = VTERM_COLOR_DEFAULT_FG;
    };

    int sb_size = (int)m_scrollback_buffer.size();
    int rows, cols;
    vterm_get_size(m_vt, &rows, &cols);

    VTermScreenCell vcell;
    clear_cell(vcell);

    if (row < sb_size) {
        // En scrollback. m_scrollback_buffer[0] es la línea más RECUPERADA (la última que salió).
        // Por lo tanto, el índice absoluto 'row' mapea a m_scrollback_buffer[sb_size - 1 - row].
        const auto& line = m_scrollback_buffer[sb_size - 1 - row];
        if (col >= 0 && col < (int)line.cells.size()) {
            vcell = line.cells[col];
        }
    } else {
        // En pantalla activa.
        VTermPos pos = { row - sb_size, col };
        if (pos.row >= 0 && pos.row < rows && col >= 0 && col < cols)
            vterm_screen_get_cell(m_screen, pos, &vcell);
    }

    return vcell;
}

Cell TerminalController::get_cell(int row, int col) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int sb_size = (int)m_scrollback_buffer.size();

    VTermScreenCell vcell = get_vterm_cell(row, col);
    bool wrapped = false;

    if (row < sb_size) {
        const auto& line = m_scrollback_buffer[sb_size - 1 - row];
        wrapped = line.wrapped;
    } else {
        VTermState* state = vterm_obtain_state(m_vt);
        const VTermLineInfo* info = vterm_state_get_lineinfo(state, row - sb_size);
        wrapped = info ? info->continuation : false;
    }

    Cell cell;
    cell.wrapped = wrapped;
    cell.is_continuation = (vcell.width == 0);

    std::stringstream ss;
    for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && vcell.chars[i] != 0; ++i) {
        ss << utf32_to_utf8(vcell.chars[i]);
    }
    cell.text = ss.str();
    if (cell.text.empty() && !cell.is_continuation) {
        cell.text = " ";
    }

    return cell;
}

int TerminalController::get_total_rows() const {
    int rows, cols;
    vterm_get_size(m_vt, &rows, &cols);
    return (int)m_scrollback_buffer.size() + rows;
}

} // namespace terminal
} // namespace horizon
