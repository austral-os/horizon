#include <horizon/text/TextDocument.hpp>
#include <horizon/Logger.hpp>
#include <fstream>
#include <codecvt>
#include <locale>
#include <algorithm>

// 1. Define basic types needs for stb_textedit header mode
#define STB_TEXTEDIT_CHARTYPE char32_t
#define STB_TEXTEDIT_STRING horizon::text::TextDocument

// 2. Include stb_textedit in header mode (no IMPLEMENTATION defined yet)
#include <horizon/external/stb_textedit.h>

namespace horizon {
namespace text {

// Implementations for width/layout - These work now as StbTexteditRow is defined
static float get_width_func(TextDocument* str, int n, int i) {
    return 10.0f; // Placeholder
}

static void layout_func(StbTexteditRow* row, TextDocument* str, int start_i) {
    int len = str->get_length();
    int i = start_i;
    int count = 0;
    while (i < len && str->get_char(i) != '\n') {
        i++;
        count++;
    }
    if (i < len && str->get_char(i) == '\n') count++;

    row->num_chars = count;
    row->x0 = 0;
    row->x1 = (float)count * 10.0f;
    row->baseline_y_delta = 20.0f;
    row->ymin = -15.0f;
    row->ymax = 5.0f;
}

} // namespace text
} // namespace horizon

// 3. Define all bridge macros
#define STB_TEXTEDIT_STRINGLEN(obj) ((obj)->get_length())
#define STB_TEXTEDIT_GETCHAR(obj, i) ((obj)->get_char(i))
#define STB_TEXTEDIT_GETWIDTH(obj, n, i) (horizon::text::get_width_func(obj, n, i))
#define STB_TEXTEDIT_LAYOUTROW(row, obj, n) (horizon::text::layout_func(row, obj, n))
#define STB_TEXTEDIT_DELETECHARS(obj, i, n) ((obj)->delete_chars(i, n))
#define STB_TEXTEDIT_INSERTCHARS(obj, i, c, n) ((obj)->insert_chars(i, c, n))

#define STB_TEXTEDIT_KEYTOTEXT(k) (((k) < 0x100000) ? (k) : -1)

#define STB_TEXTEDIT_K_SHIFT 0x200000
#define STB_TEXTEDIT_K_CONTROL 0x400000

#define STB_TEXTEDIT_K_LEFT (0x100001)
#define STB_TEXTEDIT_K_RIGHT (0x100002)
#define STB_TEXTEDIT_K_UP (0x100003)
#define STB_TEXTEDIT_K_DOWN (0x100004)
#define STB_TEXTEDIT_K_LINESTART (0x100005)
#define STB_TEXTEDIT_K_LINEEND (0x100006)
#define STB_TEXTEDIT_K_TEXTSTART (0x100007)
#define STB_TEXTEDIT_K_TEXTEND (0x100008)
#define STB_TEXTEDIT_K_DELETE (0x100009)
#define STB_TEXTEDIT_K_BACKSPACE (0x10000A)
#define STB_TEXTEDIT_K_UNDO (0x10000B)
#define STB_TEXTEDIT_K_REDO (0x10000C)
#define STB_TEXTEDIT_K_WORDLEFT (0x10000D)
#define STB_TEXTEDIT_K_WORDRIGHT (0x10000E)
#define STB_TEXTEDIT_K_PGUP (0x10000F)
#define STB_TEXTEDIT_K_PGDOWN (0x100010)

#define STB_TEXTEDIT_IS_SPACE(ch) (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
#define STB_TEXTEDIT_NEWLINE '\n'

// 4. Include stb_textedit in implementation mode
#define STB_TEXTEDIT_IMPLEMENTATION
#include <horizon/external/stb_textedit.h>

namespace horizon {
namespace text {

TextDocument::TextDocument() {
    m_state = std::make_unique<STB_TexteditState>();
    stb_textedit_initialize_state(m_state.get(), 0);
}

TextDocument::~TextDocument() {}

void TextDocument::set_text(const std::string& utf8_text) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    try {
        m_data = converter.from_bytes(utf8_text);
    } catch (...) {
        LOG_ERROR << "Failed to convert UTF-8 to UTF-32";
        m_data = U"Error decoding file";
    }
    stb_textedit_initialize_state(m_state.get(), 0);
    m_is_dirty = false;
    if (on_changed) on_changed();
}

std::string TextDocument::get_text() const {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.to_bytes(m_data);
}

bool TextDocument::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    set_text(content);
    m_path = path;
    m_is_dirty = false;
    return true;
}

bool TextDocument::save_to_file(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    file << get_text();
    m_path = path;
    m_is_dirty = false;
    return true;
}

void TextDocument::delete_chars(int index, int n) {
    if (index >= 0 && index + n <= (int)m_data.size()) {
        m_data.erase(index, n);
        m_is_dirty = true;
        if (on_changed) on_changed();
    }
}

bool TextDocument::insert_chars(int index, const char32_t* chars, int n) {
    if (index >= 0 && index <= (int)m_data.size()) {
        m_data.insert(index, chars, n);
        m_is_dirty = true;
        if (on_changed) on_changed();
        return true;
    }
    return false;
}

void TextDocument::undo() {
    stb_text_undo(this, m_state.get());
    if (on_changed) on_changed();
}

void TextDocument::redo() {
    stb_text_redo(this, m_state.get());
    if (on_changed) on_changed();
}

void TextDocument::handle_key(int key) {
    // Map our EditorKey enum to STB_TEXTEDIT_K flags
    int stb_key = key;
    if (key == (int)EditorKey::Left) stb_key = STB_TEXTEDIT_K_LEFT;
    else if (key == (int)EditorKey::Right) stb_key = STB_TEXTEDIT_K_RIGHT;
    else if (key == (int)EditorKey::Up) stb_key = STB_TEXTEDIT_K_UP;
    else if (key == (int)EditorKey::Down) stb_key = STB_TEXTEDIT_K_DOWN;
    else if (key == (int)EditorKey::LineStart) stb_key = STB_TEXTEDIT_K_LINESTART;
    else if (key == (int)EditorKey::LineEnd) stb_key = STB_TEXTEDIT_K_LINEEND;
    else if (key == (int)EditorKey::BackSpace) stb_key = STB_TEXTEDIT_K_BACKSPACE;
    else if (key == (int)EditorKey::Delete) stb_key = STB_TEXTEDIT_K_DELETE;
    else if (key == (int)EditorKey::Undo) stb_key = STB_TEXTEDIT_K_UNDO;
    else if (key == (int)EditorKey::Redo) stb_key = STB_TEXTEDIT_K_REDO;

    stb_textedit_key(this, m_state.get(), stb_key);
    if (on_changed) on_changed();
}

void TextDocument::handle_click(double x, double y) {
    stb_textedit_click(this, m_state.get(), (float)x, (float)y);
    if (on_changed) on_changed();
}

void TextDocument::handle_drag(double x, double y) {
    stb_textedit_drag(this, m_state.get(), (float)x, (float)y);
    if (on_changed) on_changed();
}

int TextDocument::get_cursor_pos() const {
    return m_state->cursor;
}

int TextDocument::get_selection_start() const {
    return m_state->select_start;
}

int TextDocument::get_selection_end() const {
    return m_state->select_end;
}

} // namespace text
} // namespace horizon
