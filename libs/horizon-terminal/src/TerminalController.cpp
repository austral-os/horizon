#include "TerminalController.hpp"
#include <iostream>
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
        .sb_pushline = nullptr,
        .sb_popline = nullptr,
        .sb_clear = nullptr
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

} // namespace terminal
} // namespace horizon
