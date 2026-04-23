#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

// Forward declaration of stb_textedit state
struct STB_TexteditState;

namespace horizon {
namespace text {

// Special key constants for portable input handling
enum class EditorKey {
    Left = 0x100001,
    Right = 0x100002,
    Up = 0x100003,
    Down = 0x100004,
    LineStart = 0x100005,
    LineEnd = 0x100006,
    BackSpace = 0x100007,
    Delete = 0x100008,
    Undo = 0x100009,
    Redo = 0x10000A,
    Shift = 0x200000,
    Control = 0x400000
};

/**
 * @class TextDocument
 * @brief Represents a text file being edited.
 * 
 * This class manages the raw text data, undo/redo history, and provides 
 * the necessary interface for stb_textedit.
 */
class TextDocument {
public:
    TextDocument();
    ~TextDocument();

    // Data management
    void set_text(const std::string& utf8_text);
    std::string get_text() const;

    bool load_from_file(const std::string& path);
    bool save_to_file(const std::string& path);

    // stb_textedit integration
    STB_TexteditState* get_state() { return m_state.get(); }
    
    // Public API for editing (wraps stb_textedit)
    void handle_key(int key);
    void handle_click(double x, double y);
    void handle_drag(double x, double y);
    void set_cursor_at_index(int index, bool select);
    void set_selection(int start, int end);

    int get_cursor_pos() const;
    int get_selection_start() const;
    int get_selection_end() const;
    std::string get_selected_text() const;

    void get_cursor_row_col(int& row, int& col) const;
    int get_line_count() const;
    
    // Callbacks helpers (used by the bridge)
    int get_length() const { return (int)m_data.size(); }
    char32_t get_char(int index) const { return m_data[index]; }
    
    void delete_chars(int index, int n);
    bool insert_chars(int index, const char32_t* chars, int n);
    void insert_text(const std::string& utf8_text);

    // Undo/Redo
    void undo();
    void redo();

    // Dirty state
    bool is_dirty() const { return m_is_dirty; }
    void clear_dirty() { m_is_dirty = false; }

    // Path
    void set_path(const std::string& path) { m_path = path; }
    const std::string& get_path() const { return m_path; }
    
    // Layout metrics for stb_textedit (public so the bridge can see them)
    float m_line_height = 20.0f;
    float m_ascent = 15.0f;
    float m_char_width = 10.0f;

    void set_metrics(float line_height, float ascent, float char_width) {
        m_line_height = line_height;
        m_ascent = ascent;
        m_char_width = char_width;
    }

    // Signal for changes
    std::function<void()> on_changed;

    const std::u32string& get_data() const { return m_data; }

    struct LineMetric {
        float y_offset;
        float height;
        size_t start_byte;
        size_t end_byte;
    };
    void set_line_metrics(const std::vector<LineMetric>& metrics) { m_line_metrics = metrics; }
    const std::vector<LineMetric>& get_line_metrics() const { return m_line_metrics; }

    uint64_t get_version() const { return m_version; }
    
    mutable std::recursive_mutex m_mutex;
    std::u32string m_data; // UTF-32 internal storage for easy indexing
    std::vector<LineMetric> m_line_metrics;
    std::unique_ptr<STB_TexteditState> m_state;
    uint64_t m_version = 0;
    std::string m_path;
    bool m_is_dirty = false;
};

} // namespace text
} // namespace horizon
