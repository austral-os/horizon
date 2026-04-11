#include "TerminalController.hpp"
#include <cstring>
#include <algorithm>
#include <sstream>

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
}

void TerminalController::write_to_pty(const char* data, size_t len) {
    // This is handled by the widget/pty connection
}

void TerminalController::resize(int rows, int cols) {
    std::lock_guard<std::mutex> lock(m_mutex);
    vterm_set_size(m_vt, rows, cols);
}

// Callbacks
int TerminalController::screen_damage(VTermRect rect, void *user) {
    auto self = static_cast<TerminalController*>(user);
    if (self->m_damage_cb) {
        self->m_damage_cb(rect);
    }
    return 0;
}

int TerminalController::screen_moverect(VTermRect dest, VTermRect src, void *user) {
    auto self = static_cast<TerminalController*>(user);
    // For now, we simple damage the whole dest area
    if (self->m_damage_cb) {
        self->m_damage_cb(dest);
    }
    return 0;
}

int TerminalController::screen_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user) {
    auto self = static_cast<TerminalController*>(user);
    if (self->m_move_cursor_cb) {
        self->m_move_cursor_cb(pos);
    }
    return 0;
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
    }
    return 0;
}

int TerminalController::screen_bell(void *user) {
    return 0;
}

int TerminalController::screen_resize(int rows, int cols, void *user) {
    return 0;
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
    
    if (self->m_scrollback_buffer.empty()) {
        return 0;
    }
    
    auto& line = self->m_scrollback_buffer.front();
    int copy_cols = std::min(cols, (int)line.cells.size());
    std::copy(line.cells.begin(), line.cells.begin() + copy_cols, cells);
    
    self->m_scrollback_buffer.pop_front();
    return 1;
}


int TerminalController::screen_sb_clear(void *user) {
    auto self = static_cast<TerminalController*>(user);
    self->m_scrollback_buffer.clear();
    return 1;
}

Cell TerminalController::get_cell(int row, int col) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int sb_size = (int)m_scrollback_buffer.size();
    int rows, cols;
    vterm_get_size(m_vt, &rows, &cols);

    VTermScreenCell vcell;
    bool wrapped = false;

    if (row < sb_size) {
        // En scrollback. m_scrollback_buffer[0] es la línea más RECUPERADA (la última que salió).
        // Por lo tanto, el índice absoluto 'row' mapea a m_scrollback_buffer[sb_size - 1 - row].
        const auto& line = m_scrollback_buffer[sb_size - 1 - row];
        if (col >= 0 && col < (int)line.cells.size()) {
            vcell = line.cells[col];
        } else {
            memset(&vcell, 0, sizeof(vcell));
        }
        wrapped = line.wrapped;
    } else {
        // En pantalla activa.
        VTermPos pos = { row - sb_size, col };
        if (vterm_screen_get_cell(m_screen, pos, &vcell) == 0) {
            memset(&vcell, 0, sizeof(vcell));
        }
        VTermState* state = vterm_obtain_state(m_vt);
        const VTermLineInfo* info = vterm_state_get_lineinfo(state, pos.row);
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

