#pragma once

#include <vterm.h>
#include "PtyHandler.hpp"
#include <memory>
#include <functional>
#include <mutex>
#include <deque>
#include <vector>
#include <string>


namespace horizon {
namespace terminal {

struct BufferPos {
    int row; // absolute row (scrollback + visible screen)
    int col;
};

struct Cell {
    std::string text;     // UTF-8 character(s)
    bool is_continuation; // true if part of a wide character
    bool wrapped;         // true if logically continued from previous line
};

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

    // Scrollback buffer support
    void set_scrollback_limit(size_t limit) { m_scrollback_limit = limit; }
    struct ScrollbackLine {
        std::vector<VTermScreenCell> cells;
        bool wrapped;
    };
    size_t get_scrollback_size() const { return m_scrollback_buffer.size(); }
    const ScrollbackLine& get_scrollback_line(size_t index) const { return m_scrollback_buffer[index]; }
 
    Cell get_cell(int row, int col);

    int get_total_rows() const;


private:
    std::deque<ScrollbackLine> m_scrollback_buffer;
    size_t m_scrollback_limit = 5000;

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
    static int screen_sb_pushline(int cols, const VTermScreenCell *cells, void *user);
    static int screen_sb_popline(int cols, VTermScreenCell *cells, void *user);
    static int screen_sb_clear(void *user);
};

} // namespace terminal
} // namespace horizon
