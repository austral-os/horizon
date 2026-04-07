#pragma once

#include <vterm.h>
#include "PtyHandler.hpp"
#include <memory>
#include <functional>
#include <mutex>

namespace horizon {
namespace terminal {

class TerminalController {
public:
    TerminalController(int rows, int cols);
    ~TerminalController();

    void push_data(const char* data, size_t len);
    void write_to_pty(const char* data, size_t len);
    void resize(int rows, int cols);

    VTerm* get_vterm() { return m_vt; }
    VTermScreen* get_screen() { return m_screen; }

    // Callback for when the screen needs redrawing
    void set_damage_callback(std::function<void(VTermRect)> cb) { m_damage_cb = cb; }
    void set_move_cursor_callback(std::function<void(VTermPos)> cb) { m_move_cursor_cb = cb; }

private:
    VTerm* m_vt;
    VTermScreen* m_screen;
    std::mutex m_mutex;

    std::function<void(VTermRect)> m_damage_cb;
    std::function<void(VTermPos)> m_move_cursor_cb;

    static int screen_damage(VTermRect rect, void *user);
    static int screen_moverect(VTermRect dest, VTermRect src, void *user);
    static int screen_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int screen_settermprop(VTermProp prop, VTermValue *val, void *user);
    static int screen_bell(void *user);
    static int screen_resize(int rows, int cols, void *user);
};

} // namespace terminal
} // namespace horizon
