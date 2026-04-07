#include "TerminalController.hpp"
#include <cstring>

namespace horizon {
namespace terminal {

TerminalController::TerminalController(int rows, int cols) {
    m_vt = vterm_new(rows, cols);
    vterm_set_utf8(m_vt, 1);
    m_screen = vterm_obtain_screen(m_vt);
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
    return 1;
}

int TerminalController::screen_moverect(VTermRect dest, VTermRect src, void *user) {
    auto self = static_cast<TerminalController*>(user);
    // For now, we simple damage the whole dest area
    if (self->m_damage_cb) {
        self->m_damage_cb(dest);
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
    
    std::vector<VTermScreenCell> line(cells, cells + cols);
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
    int copy_cols = std::min(cols, (int)line.size());
    std::copy(line.begin(), line.begin() + copy_cols, cells);
    
    self->m_scrollback_buffer.pop_front();
    return 1;
}

int TerminalController::screen_sb_clear(void *user) {
    auto self = static_cast<TerminalController*>(user);
    self->m_scrollback_buffer.clear();
    return 1;
}

} // namespace terminal
} // namespace horizon
