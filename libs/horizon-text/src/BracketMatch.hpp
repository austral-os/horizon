#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace horizon {
namespace text {
namespace detail {

// -----------------------------------------------------------------------
// Bracket pair matching helpers
//
// These are extracted as inline functions so they can be unit-tested
// without instantiating a TextEditorWidget / Cairo surface.
// They are NOT part of the public API — include from internal tests only.
// -----------------------------------------------------------------------

inline bool is_opening_bracket(char32_t c) {
    return c == '(' || c == '[' || c == '{';
}

inline bool is_closing_bracket(char32_t c) {
    return c == ')' || c == ']' || c == '}';
}

inline char32_t matching_bracket(char32_t c) {
    switch (c) {
        case '(': return ')';
        case ')': return '(';
        case '[': return ']';
        case ']': return '[';
        case '{': return '}';
        case '}': return '{';
        default: return 0;
    }
}

// Forward search: find matching closing bracket with nesting support.
// Returns index of the matching bracket, or -1 if not found within max_chars.
inline int find_matching_forward(const std::u32string& text, int pos,
                                 char32_t open, char32_t close,
                                 size_t max_chars) {
    int depth = 1;
    size_t end = std::min(static_cast<size_t>(pos) + max_chars, text.size());
    for (size_t i = static_cast<size_t>(pos) + 1; i < end; ++i) {
        if (text[i] == open) {
            ++depth;
        } else if (text[i] == close) {
            --depth;
            if (depth == 0) return static_cast<int>(i);
        }
    }
    return -1;
}

// Backward search: find matching opening bracket with nesting support.
// Returns index of the matching bracket, or -1 if not found within max_chars.
inline int find_matching_backward(const std::u32string& text, int pos,
                                  char32_t open, char32_t close,
                                  size_t max_chars) {
    int depth = 1;
    int start = std::max(0, pos - static_cast<int>(max_chars));
    for (int i = pos - 1; i >= start; --i) {
        if (text[i] == close) {
            ++depth;
        } else if (text[i] == open) {
            --depth;
            if (depth == 0) return i;
        }
    }
    return -1;
}

} // namespace detail
} // namespace text
} // namespace horizon
