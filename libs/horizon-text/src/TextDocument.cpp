#include <horizon/text/TextDocument.hpp>
#include <horizon/Logger.hpp>
#include <fstream>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <mutex>

// 1. Define basic types needs for stb_textedit header mode
#define STB_TEXTEDIT_CHARTYPE char32_t
#define STB_TEXTEDIT_STRING horizon::text::TextDocument

// 2. Include stb_textedit in header mode (no IMPLEMENTATION defined yet)
#include <horizon/external/stb_textedit.h>

namespace horizon {
namespace text {

// Implementations for width/layout - These work now as StbTexteditRow is defined
static float get_width_func(TextDocument* str, int n, int i) {
    return str->m_char_width;
}

static void layout_func(StbTexteditRow* row, TextDocument* str, int start_i) {
    int len = str->get_length();
    int i = start_i;
    int count = 0;
    
    // Calculate which line we are on
    int line_index = 0;
    for (int j = 0; j < start_i && j < len; ++j) {
        if (str->get_char(j) == '\n') line_index++;
    }
    
    while (i < len && str->get_char(i) != '\n') {
        i++;
        count++;
    }
    if (i < len && str->get_char(i) == '\n') count++;

    float y_offset = 0;
    float height = str->m_line_height;
    
    const auto& metrics = str->get_line_metrics();
    if (line_index < (int)metrics.size()) {
        y_offset = metrics[line_index].y_offset;
        height = metrics[line_index].height;
    } else if (!metrics.empty()) {
        // Fallback for lines beyond current metrics (should not happen if synchronized)
        y_offset = metrics.back().y_offset + metrics.back().height + (line_index - (int)metrics.size() + 1) * str->m_line_height;
    } else {
        y_offset = (float)line_index * str->m_line_height;
    }

    row->num_chars = count;
    row->x0 = 0;
    row->x1 = (float)count * str->m_char_width;
    row->baseline_y_delta = str->m_line_height * 0.8f; // Reasonable estimate
    row->ymin = y_offset;
    row->ymax = y_offset + height;
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

void TextDocument::set_text(const std::string& text) {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_data.clear();
        const unsigned char* p = (const unsigned char*)text.c_str();
        while (*p) {
            if ((*p & 0x80) == 0) {
                m_data.push_back(*p++);
            } else if ((*p & 0xE0) == 0xC0) {
                char32_t c = (*p++ & 0x1F) << 6;
                if (*p && (*p & 0xC0) == 0x80) c |= (*p++ & 0x3F);
                m_data.push_back(c);
            } else if ((*p & 0xF0) == 0xE0) {
                char32_t c = (*p++ & 0x0F) << 12;
                if (*p && (*p & 0xC0) == 0x80) c |= (*p++ & 0x3F) << 6;
                if (*p && (*p & 0xC0) == 0x80) c |= (*p++ & 0x3F);
                m_data.push_back(c);
            } else if ((*p & 0xF8) == 0xF0) {
                char32_t c = (*p++ & 0x07) << 18;
                if (*p && (*p & 0xC0) == 0x80) c |= (*p++ & 0x3F) << 12;
                if (*p && (*p & 0xC0) == 0x80) c |= (*p++ & 0x3F) << 6;
                if (*p && (*p & 0xC0) == 0x80) c |= (*p++ & 0x3F);
                m_data.push_back(c);
            } else {
                p++; 
            }
        }
        stb_textedit_initialize_state(m_state.get(), 0);
        m_is_dirty = false;
        m_version++;
    }
    if (on_changed) on_changed();
}

std::string TextDocument::get_text() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::string result;
    result.reserve(m_data.length() * 2); // Better estimate
    for (char32_t c : m_data) {
        if (c <= 0x7F) {
            result.push_back((char)c);
        } else if (c <= 0x7FF) {
            result.push_back((char)(0xC0 | (c >> 6)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        } else if (c <= 0xFFFF) {
            result.push_back((char)(0xE0 | (c >> 12)));
            result.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        } else if (c <= 0x10FFFF) {
            result.push_back((char)(0xF0 | (c >> 18)));
            result.push_back((char)(0x80 | ((c >> 12) & 0x3F)));
            result.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        }
    }
    return result;
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
    if (path.empty()) return false;
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    file << get_text();
    m_path = path;
    m_is_dirty = false;
    return true;
}

void TextDocument::delete_chars(int index, int n) {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (index >= 0 && index + n <= (int)m_data.size()) {
            m_data.erase(index, n);
            m_is_dirty = true; m_version++;
        }
    }
    if (on_changed) on_changed();
}

bool TextDocument::insert_chars(int index, const char32_t* chars, int n) {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (index >= 0 && index <= (int)m_data.size()) {
            m_data.insert(index, chars, n);
            m_is_dirty = true; m_version++;
        } else {
            return false;
        }
    }
    if (on_changed) on_changed();
    return true;
}

void TextDocument::undo() {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        stb_text_undo(this, m_state.get());
    }
    if (on_changed) on_changed();
}

void TextDocument::redo() {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        stb_text_redo(this, m_state.get());
    }
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

    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        stb_textedit_key(this, m_state.get(), stb_key);
    }
    if (on_changed) on_changed();
}

void TextDocument::handle_click(double x, double y) {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        stb_textedit_click(this, m_state.get(), (float)x, (float)y);
    }
    if (on_changed) on_changed();
}

void TextDocument::handle_drag(double x, double y) {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        stb_textedit_drag(this, m_state.get(), (float)x, (float)y);
    }
    if (on_changed) on_changed();
}

void TextDocument::set_cursor_at_index(int index, bool select) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (index < 0) index = 0;
    if (index > (int)m_data.length()) index = (int)m_data.length();
    
    m_state->cursor = index;
    if (!select) {
        m_state->select_start = index;
        m_state->select_end = index;
    } else {
        m_state->select_end = index;
    }
}

void TextDocument::set_selection(int start, int end) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (start < 0) start = 0;
    if (start > (int)m_data.length()) start = (int)m_data.length();
    if (end < 0) end = 0;
    if (end > (int)m_data.length()) end = (int)m_data.length();
    
    m_state->select_start = start;
    m_state->select_end = end;
}

int TextDocument::get_cursor_pos() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_state->cursor;
}

int TextDocument::get_selection_start() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return std::min(m_state->select_start, m_state->select_end);
}

int TextDocument::get_selection_end() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return std::max(m_state->select_start, m_state->select_end);
}

std::string TextDocument::get_selected_text() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    int start = get_selection_start();
    int end = get_selection_end();
    int s = std::min(start, end);
    int e = std::max(start, end);
    if (s < 0) s = 0;
    if (e > (int)m_data.size()) e = (int)m_data.size();
    if (s >= e) return "";

    std::u32string sub = m_data.substr(s, e - s);
    
    std::string result;
    result.reserve(sub.length());
    for (char32_t c : sub) {
        if (c <= 0x7F) {
            result.push_back((char)c);
        } else if (c <= 0x7FF) {
            result.push_back((char)(0xC0 | (c >> 6)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        } else if (c <= 0xFFFF) {
            result.push_back((char)(0xE0 | (c >> 12)));
            result.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        } else if (c <= 0x10FFFF) {
            result.push_back((char)(0xF0 | (c >> 18)));
            result.push_back((char)(0x80 | ((c >> 12) & 0x3F)));
            result.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
            result.push_back((char)(0x80 | (c & 0x3F)));
        }
    }
    return result;
}

void TextDocument::get_cursor_row_col(int& row, int& col) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    row = 1;
    col = 1;
    int cursor = m_state->cursor;
    for (int i = 0; i < cursor && i < (int)m_data.size(); ++i) {
        if (m_data[i] == '\n') {
            row++;
            col = 1;
        } else {
            col++;
        }
    }
}

int TextDocument::get_line_count() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    int count = 1;
    for (char32_t c : m_data) {
        if (c == '\n') count++;
    }
    return count;
}

void TextDocument::insert_text(const std::string& utf8_text) {
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        std::u32string u32_text;
        const unsigned char* p = (const unsigned char*)utf8_text.c_str();
        while (*p) {
            if ((*p & 0x80) == 0) u32_text.push_back(*p++);
            else if ((*p & 0xE0) == 0xC0) { char32_t c = (*p++ & 0x1F) << 6; if (*p) c |= (*p++ & 0x3F); u32_text.push_back(c); }
            else if ((*p & 0xF0) == 0xE0) { char32_t c = (*p++ & 0x0F) << 12; if (*p) c |= (*p++ & 0x3F) << 6; if (*p) c |= (*p++ & 0x3F); u32_text.push_back(c); }
            else if ((*p & 0xF8) == 0xF0) { char32_t c = (*p++ & 0x07) << 18; if (*p) c |= (*p++ & 0x3F) << 12; if (*p) c |= (*p++ & 0x3F) << 6; if (*p) c |= (*p++ & 0x3F); u32_text.push_back(c); }
            else p++;
        }
        
        stb_textedit_paste(this, m_state.get(), u32_text.c_str(), (int)u32_text.length());
    }
    if (on_changed) on_changed();
}

} // namespace text
} // namespace horizon
