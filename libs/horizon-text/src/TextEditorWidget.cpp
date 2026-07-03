#include <horizon/text/TextEditorWidget.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Application.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Logger.hpp>
#include <linux/input-event-codes.h>
#include <algorithm>
#include <climits>
#include <codecvt>
#include <locale>
#include <cairo/cairo-pdf.h>


namespace horizon {
namespace text {

TextEditorWidget::TextEditorWidget() : Widget() {
    set_focusable(true);
    set_cursor_type(CursorType::Text);
    set_size(800, 600); // Ensure non-zero initial size
    m_last_blink = std::chrono::steady_clock::now();

    m_highlighter = std::make_unique<SyntaxHighlighter>();
    m_highlighter->load_default_cpp_rules();

    when_key_press.connect([this](KeyEventContext& ev) { this->handle_key_event(ev); });
    when_mouse_press.connect([this](MouseButtonEventContext& ev) { this->handle_mouse_event(ev); });
    when_mouse_drag.connect([this](MouseMoveEventContext& ev) { this->handle_mouse_drag(ev); });
    
    when_undo.connect([this](EventContext&) {
        if (m_doc) {
            m_doc->undo();
            invalidate();
            m_needs_ensure_visible = true;
            m_cursor_visible = true;
            m_last_blink = std::chrono::steady_clock::now();
        }
    });

    when_redo.connect([this](EventContext&) {
        if (m_doc) {
            m_doc->redo();
            invalidate();
            m_needs_ensure_visible = true;
            m_cursor_visible = true;
            m_last_blink = std::chrono::steady_clock::now();
        }
    });
    
    when_application_load.connect([this](EventContext&) {
        // No manual timer needed, WaylandWindow handles cursor blink 
        // by invalidating the focused widget every 500ms.
    });

    when_focus.connect([this](EventContext&) {
        m_cursor_visible = true;
        m_last_blink = std::chrono::steady_clock::now();
        invalidate();
    });
}

TextEditorWidget::~TextEditorWidget() {
    if (m_layout) g_object_unref(m_layout);
    if (m_doc && m_doc->on_changed) {
        m_doc->on_changed = nullptr;
    }
}

void TextEditorWidget::set_document(std::shared_ptr<TextDocument> doc) {
    if (m_doc && m_doc->on_changed) {
        m_doc->on_changed = nullptr;
    }
    m_doc = doc;
    if (m_doc) {
        m_doc->on_changed = [this]() {
            m_metrics_dirty = true;
            this->calculate_layout();
            this->invalidate();
        };
        if (m_parent) {
            ensure_metrics();
            calculate_layout();
        }
    }
    invalidate();
}

// ---------------------------------------------------------------------------
// ensure_byte_offsets: rebuild the UTF-32 -> UTF-8 byte-offset table only
// when the document version changed. O(n) on first call, O(1) thereafter.
// ---------------------------------------------------------------------------
void TextEditorWidget::ensure_byte_offsets() {
    if (!m_doc) return;
    uint64_t ver = m_doc->get_version();
    if (ver == m_byte_offsets_version) return;

    const std::u32string& u32 = m_doc->get_data();
    m_byte_offsets.clear();
    m_byte_offsets.reserve(u32.size() + 1);
    size_t byte = 0;
    for (char32_t c : u32) {
        m_byte_offsets.push_back(byte);
        if      (c <= 0x7F)   byte += 1;
        else if (c <= 0x7FF)  byte += 2;
        else if (c <= 0xFFFF) byte += 3;
        else                  byte += 4;
    }
    m_byte_offsets.push_back(byte);
    m_byte_offsets_version = ver;
}

void TextEditorWidget::draw(GraphicsContext& gc) {
    if (!m_doc) return;

    cairo_t* cr = (cairo_t*)gc.getNativeContext();
    if (!cr) return;

    if (!application()) return;
    auto* tm = theme_manager();

    // 1. Rebuild Pango layout + line metrics only when document or scroll changed.
    bool version_changed = (m_doc->get_version() != m_last_layout_version);
    int current_scroll_y = 0;
    auto* scroll = dynamic_cast<ScrollArea*>(m_parent);
    if (scroll) {
        current_scroll_y = scroll->scroll_y();
    }

    if (m_metrics_dirty || version_changed) {
        ensure_metrics();
        m_metrics_dirty = false;
        // Update widget size
        float line_height = m_doc->m_line_height;
        if (line_height < 1.0f) line_height = m_font_size;
        int text_h = m_doc->get_line_count() * line_height;
        
        int max_chars = 0;
        const auto& line_starts = m_doc->ensure_line_index();
        for (size_t i = 0; i < line_starts.size(); ++i) {
            int len = 0;
            if (i + 1 < line_starts.size()) len = line_starts[i + 1] - line_starts[i];
            else len = (int)m_doc->get_length() - line_starts[i];
            max_chars = std::max(max_chars, len);
        }
        float char_width = m_doc->m_char_width;
        if (char_width < 1.0f) char_width = m_font_size * 0.6f;
        int text_w = max_chars * char_width;

        int margin_x = (m_show_line_numbers ? m_line_number_margin + 5 : 5);
        int target_w = std::max(text_w + margin_x + 5, m_parent ? m_parent->width()  : 800);
        int target_h = std::max(text_h + 5 + 5,        m_parent ? m_parent->height() : 600);
        if (target_w != m_width || target_h != m_height) {
            set_size(target_w, target_h);
        }
    }

    // Always update visible layout to match current scroll/view window
    update_pango_layout(cr);
    if (!m_layout) return;

    // 2. Rebuild byte-offset cache (O(1) if version unchanged)
    ensure_byte_offsets();

    // 3. Snapshot cursor / selection state
    int cursor_pos = 0;
    int sel_start = 0;
    int sel_end = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(m_doc->m_mutex);
        cursor_pos = m_doc->get_cursor_pos();
        sel_start  = m_doc->get_selection_start();
        sel_end    = m_doc->get_selection_end();
    }

    // 4. Cursor blink
    auto now = std::chrono::steady_clock::now();
    auto elapsed_blink = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_blink).count();
    if (elapsed_blink >= 500) {
        m_cursor_visible = !m_cursor_visible;
        m_last_blink = now;
    }

    // 5. Clear background
    Color bg_color = tm->get_color("textbox_bg");
    gc.setColor(bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    gc.fillRect(m_x, m_y, m_width, m_height);

    pango_cairo_update_layout(cr, m_layout);

    // 6. Calculate margins
    int margin_x = (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int margin_y = 5;
    int tx = m_x + margin_x;
    int ty = m_y + margin_y;

    float line_height = m_doc->m_line_height;
    if (line_height < 1.0f) line_height = m_font_size;

    // Visible line range matching current layout slice exactly
    int vis_first_line = m_vis_first_line;
    int vis_last_line = m_vis_last_line;

    // 7. Highlight current line
    if (m_highlight_current_line) {
        int cursor_row, cursor_col;
        m_doc->get_cursor_row_col(cursor_row, cursor_col);
        int cursor_row_0 = cursor_row - 1;
        if (cursor_row_0 >= vis_first_line && cursor_row_0 <= vis_last_line) {
            Color line_color = tm->get_color("textbox_bg");
            line_color = tm->is_dark() ? line_color.lighter(8.0f) : line_color.darker(8.0f);
            line_color = line_color.with_alpha(0.85f);
            cairo_set_source_rgba(cr, line_color.r, line_color.g, line_color.b, line_color.a);
            cairo_rectangle(cr, tx, ty + cursor_row_0 * line_height, std::max(0, m_width - margin_x), line_height);
            cairo_fill(cr);
        }
    }

    // 8. Draw Selection Highlight (behind text)
    if (sel_start != sel_end) {
        int r_start   = std::min(sel_start, sel_end);
        int r_end     = std::max(sel_start, sel_end);

        int row_start, col_start;
        int row_end, col_end;
        m_doc->get_row_col_for_index(r_start, row_start, col_start);
        m_doc->get_row_col_for_index(r_end, row_end, col_end);

        Color sel_bg = tm->get_color("menu_item_selected_bg1");
        cairo_set_source_rgba(cr, sel_bg.r, sel_bg.g, sel_bg.b, sel_bg.a);

        for (int row = std::max(vis_first_line, row_start); row <= std::min(vis_last_line, row_end); ++row) {
            int x1 = 0;
            if (row == row_start) {
                x1 = get_pixel_x_for_char(row, col_start);
            }
            int x2 = 0;
            if (row == row_end) {
                x2 = get_pixel_x_for_char(row, col_end);
            } else {
                int row_len = m_doc->get_line_length(row);
                x2 = get_pixel_x_for_char(row, row_len);
            }

            int ly = ty + row * line_height;
            int rw = x2 - x1;
            if (rw < 5 && row < row_end) rw = 10;
            cairo_rectangle(cr, tx + x1, ly, rw, line_height);
            cairo_fill(cr);
        }
    }

    // 9. Draw visible text line by line to prevent vertical drift
    {
        Color fg_color = tm->get_color("window_fg");
        cairo_set_source_rgba(cr, fg_color.r, fg_color.g, fg_color.b, fg_color.a);
        
        float ascent = m_doc->m_ascent;
        for (int line_num = vis_first_line; line_num <= vis_last_line && line_num < m_doc->get_line_count(); ++line_num) {
            int layout_line_idx = line_num - vis_first_line;
            PangoLayoutLine* pline = pango_layout_get_line_readonly(m_layout, layout_line_idx);
            if (pline) {
                int baseline = ty + (int)(line_num * line_height + ascent);
                cairo_move_to(cr, tx, baseline);
                pango_cairo_show_layout_line(cr, pline);
            }
        }
    }

    // 10. Draw Margin Background & Line Numbers
    if (m_show_line_numbers) {
        Color margin_bg = tm->get_color("window_bg");
        cairo_set_source_rgba(cr, margin_bg.r, margin_bg.g, margin_bg.b, margin_bg.a);
        cairo_rectangle(cr, m_x, m_y, m_line_number_margin, m_height);
        cairo_fill(cr);

        Color margin_border = tm->get_color("group_separator");
        cairo_set_source_rgba(cr, margin_border.r, margin_border.g, margin_border.b, margin_border.a);
        cairo_move_to(cr, m_x + m_line_number_margin, m_y);
        cairo_line_to(cr, m_x + m_line_number_margin, m_y + m_height);
        cairo_stroke(cr);

        PangoLayout* num_layout = pango_cairo_create_layout(cr);
        PangoFontDescription* num_desc = pango_font_description_from_string("sans 8");
        pango_layout_set_font_description(num_layout, num_desc);
        pango_font_description_free(num_desc);

        for (int line_num = vis_first_line; line_num <= vis_last_line && line_num < m_doc->get_line_count(); ++line_num) {
            int ly = ty + line_num * line_height;
            std::string num_str = std::to_string(line_num + 1);
            pango_layout_set_text(num_layout, num_str.c_str(), -1);
            int nw, nh;
            pango_layout_get_pixel_size(num_layout, &nw, &nh);
            Color num_fg = tm->get_color("window_fg").with_alpha(0.5);
            cairo_set_source_rgba(cr, num_fg.r, num_fg.g, num_fg.b, num_fg.a);
            cairo_move_to(cr, m_x + m_line_number_margin - nw - 5, ly + (line_height - nh) / 2);
            pango_cairo_show_layout(cr, num_layout);
        }
        g_object_unref(num_layout);
    }

    // 11. Draw Cursor
    if (has_focus() && m_cursor_visible) {
        int cursor_row, cursor_col;
        m_doc->get_cursor_row_col(cursor_row, cursor_col);
        int cursor_row_0 = cursor_row - 1;
        int cursor_col_0 = cursor_col - 1;
        if (cursor_row_0 >= vis_first_line && cursor_row_0 <= vis_last_line) {
            int cx = get_pixel_x_for_char(cursor_row_0, cursor_col_0);
            int cy = ty + cursor_row_0 * line_height;
            Color cursor_color = tm->get_color("window_fg");
            cairo_set_source_rgba(cr, cursor_color.r, cursor_color.g, cursor_color.b, cursor_color.a);
            cairo_rectangle(cr, tx + cx, cy, 2, line_height);
            cairo_fill(cr);
        }
    }

    if (m_needs_ensure_visible && has_focus()) {
        ensure_cursor_visible();
        m_needs_ensure_visible = false;
    }
}

void TextEditorWidget::handle_key_event(KeyEventContext& ev) {
    if (!m_doc) return;

    int key = 0;
    bool shift = ev.modifiers & 0x1;
    bool ctrl = ev.modifiers & 0x2; // Fixed CTRL modifier from 0x4 to 0x2

    // Let UI shortcuts bubble up
    if (ctrl) {
        if (ev.keysym == XKB_KEY_Tab || ev.keysym == 0xff09 || ev.keysym == 0xfe20 || ev.keysym == XKB_KEY_w || ev.keysym == XKB_KEY_W || ev.keysym == 0x77 || ev.keysym == 0x57) {
            return;
        }
    }

    switch (ev.keysym) {
        case XKB_KEY_Left: key = (int)EditorKey::Left; break;
        case XKB_KEY_Right: key = (int)EditorKey::Right; break;
        case XKB_KEY_Up: key = (int)EditorKey::Up; break;
        case XKB_KEY_Down: key = (int)EditorKey::Down; break;
        case XKB_KEY_Home: key = (int)EditorKey::LineStart; break;
        case XKB_KEY_End: key = (int)EditorKey::LineEnd; break;
        case XKB_KEY_BackSpace: key = (int)EditorKey::BackSpace; break;
        case XKB_KEY_Delete: key = (int)EditorKey::Delete; break;
        case XKB_KEY_Return: key = '\n'; break;
        case XKB_KEY_Tab: key = '\t'; break;
        default: break;
    }
    
    if (key != 0) {
        // Special keys handling
        if (shift) key |= (int)EditorKey::Shift;
        
        // Handle Undo/Redo
        if (ctrl && (ev.keysym == XKB_KEY_z || ev.keysym == XKB_KEY_Z)) {
            if (shift) m_doc->redo();
            else m_doc->undo();
        } else {
            m_doc->handle_key(key);
        }
        m_cursor_visible = true;
        m_last_blink = std::chrono::steady_clock::now();
        invalidate();
        ev.stop_propagation = true;
    } else if (!ev.text.empty()) {
        if ((unsigned char)ev.text[0] >= 32) {
            m_doc->insert_text(ev.text);
            m_cursor_visible = true;
            m_last_blink = std::chrono::steady_clock::now();
            invalidate();
            ev.stop_propagation = true;
        } else {
            return; // Let control characters bubble up for shortcuts
        }
    } else {
        return; // Nothing to process, let it bubble up
    }
    
    m_needs_ensure_visible = true;
    m_cursor_visible = true;
    m_last_blink = std::chrono::steady_clock::now();
    
    EventContext cursor_ctx;
    cursor_ctx.sender = this;
    when_cursor_moved.run(cursor_ctx);
    invalidate();
}

void TextEditorWidget::calculate_layout() {
    if (!m_doc) return;

    // Only rebuild if the document version actually changed.
    uint64_t current_version = m_doc->get_version();
    if (current_version == m_last_layout_version && m_width > 0 && m_height > 0) {
        return; // Size is already correct, nothing to do
    }

    ensure_metrics();

    float line_height = m_doc->m_line_height;
    if (line_height < 1.0f) line_height = m_font_size;

    int max_line_len = 0;
    int line_count = m_doc->get_line_count();
    for (int i = 0; i < line_count; ++i) {
        max_line_len = std::max(max_line_len, m_doc->get_line_length(i));
    }

    int text_w = (int)(max_line_len * m_doc->m_char_width);
    int text_h = (int)(line_count * line_height);

    int margin_x = (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int margin_y = 5;

    int target_w = text_w + margin_x + 5;
    int target_h = text_h + margin_y + 5;

    if (m_parent) {
        target_w = std::max(target_w, m_parent->width());
        target_h = std::max(target_h, m_parent->height());
    }
    if (target_w != m_width || target_h != m_height) {
        set_size(target_w, target_h);
    }
}

void TextEditorWidget::ensure_highlight_cache() {
    if (!m_doc) return;
    uint64_t ver = m_doc->get_version();
    if (ver == m_highlight_cache_version) return;

    int line_count = m_doc->get_line_count();
    m_highlight_cache.clear();
    m_highlight_cache.resize(line_count);
    m_highlight_cache_valid.clear();
    m_highlight_cache_valid.resize(line_count, false);
    m_highlight_cache_version = ver;
}

bool TextEditorWidget::update_pango_layout(cairo_t* cr) {
    if (!m_layout) {
        if (cr) {
            m_layout = pango_cairo_create_layout(cr);
        } else {
            cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
            cairo_t* dummy_cr = cairo_create(surface);
            m_layout = pango_cairo_create_layout(dummy_cr);
            cairo_destroy(dummy_cr);
            cairo_surface_destroy(surface);
        }
    }
    if (!m_layout || !m_doc) return false;

    uint64_t current_version = m_doc->get_version();
    int current_scroll_y = 0;
    auto* scroll = dynamic_cast<ScrollArea*>(m_parent);
    if (scroll) {
        current_scroll_y = scroll->scroll_y();
    }

    float line_height = m_doc->m_line_height;
    if (line_height < 1.0f) line_height = m_font_size;

    // Determine the visible lines range
    constexpr int HIGHLIGHT_BUFFER_LINES = 10;
    int vis_first_line = 0;
    int vis_last_line  = m_doc->get_line_count() - 1;
    if (scroll && m_doc->m_line_height > 0.5f) {
        vis_first_line = std::max(0, (int)(current_scroll_y / line_height) - HIGHLIGHT_BUFFER_LINES);
        vis_last_line  = std::min(m_doc->get_line_count() - 1, (int)((current_scroll_y + scroll->height()) / line_height) + HIGHLIGHT_BUFFER_LINES);
    }

    const auto& line_starts_chars = m_doc->ensure_line_index();
    if (line_starts_chars.empty()) return false;
    
    vis_first_line = std::min(vis_first_line, (int)line_starts_chars.size() - 1);
    vis_last_line  = std::min(vis_last_line, (int)line_starts_chars.size() - 1);

    m_vis_first_line = vis_first_line;
    m_vis_last_line = vis_last_line;

    int sel_start = m_doc->get_selection_start();
    int sel_end = m_doc->get_selection_end();

    static uint64_t last_ver = 0xFFFFFFFFFFFFFFFF;
    static int last_first = -1;
    static int last_last = -1;
    static int last_sel_start = -1;
    static int last_sel_end = -1;
    if (current_version == last_ver && vis_first_line == last_first && vis_last_line == last_last &&
        sel_start == last_sel_start && sel_end == last_sel_end) {
        return false;
    }

    // Setup font
    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, m_font_family.c_str());
    pango_font_description_set_absolute_size(desc, m_font_size * PANGO_SCALE);
    pango_font_description_set_weight(desc, m_font_weight == 1 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
    pango_layout_set_font_description(m_layout, desc);
    pango_font_description_free(desc);
    
    pango_layout_set_width(m_layout, -1);
    pango_layout_set_wrap(m_layout, PANGO_WRAP_WORD_CHAR);

    // Get text and byte offsets of the visible range by reference
    std::u32string visible_u32;
    std::vector<size_t> local_byte_offsets;
    std::string visible_utf8;
    
    {
        std::lock_guard<std::recursive_mutex> lock(m_doc->m_mutex);
        const std::u32string& u32_text = m_doc->get_data();
        
        size_t visible_start_char = line_starts_chars[vis_first_line];
        size_t visible_end_char = (vis_last_line + 1 < (int)line_starts_chars.size()) 
                                  ? line_starts_chars[vis_last_line + 1] 
                                  : u32_text.length();

        visible_u32 = u32_text.substr(visible_start_char, visible_end_char - visible_start_char);

        // Convert visible substring to UTF-8
        visible_utf8.reserve(visible_u32.length() * 2);
        local_byte_offsets.reserve(visible_u32.length() + 1);
        
        size_t current_byte = 0;
        for (char32_t c : visible_u32) {
            local_byte_offsets.push_back(current_byte);
            if (c <= 0x7F) { visible_utf8.push_back((char)c); current_byte += 1; }
            else if (c <= 0x7FF) { visible_utf8.push_back((char)(0xC0 | (c >> 6))); visible_utf8.push_back((char)(0x80 | (c & 0x3F))); current_byte += 2; }
            else if (c <= 0xFFFF) { visible_utf8.push_back((char)(0xE0 | (c >> 12))); visible_utf8.push_back((char)(0x80 | ((c >> 6) & 0x3F))); visible_utf8.push_back((char)(0x80 | (c & 0x3F))); current_byte += 3; }
            else { visible_utf8.push_back((char)(0xF0 | (c >> 18))); visible_utf8.push_back((char)(0x80 | ((c >> 12) & 0x3F))); visible_utf8.push_back((char)(0x80 | ((c >> 6) & 0x3F))); visible_utf8.push_back((char)(0x80 | (c & 0x3F))); current_byte += 4; }
        }
        local_byte_offsets.push_back(current_byte);
    }
    
    pango_layout_set_text(m_layout, visible_utf8.c_str(), -1);

    ensure_highlight_cache();

    // Apply attributes
    PangoAttrList* attrs = pango_attr_list_new();
    size_t line_start_char = 0;
    int line_number = vis_first_line;

    {
        std::lock_guard<std::recursive_mutex> lock(m_doc->m_mutex);
        const std::u32string& u32_text = m_doc->get_data();
        
        for (size_t i = 0; i <= visible_u32.length(); ++i) {
            if (i == visible_u32.length() || visible_u32[i] == '\n') {
                if (line_number < (int)m_highlight_cache.size()) {
                    if (!m_highlight_cache_valid[line_number]) {
                        size_t global_line_start = line_starts_chars[line_number];
                        size_t global_line_end = (line_number + 1 < (int)line_starts_chars.size()) 
                                                 ? line_starts_chars[line_number + 1] 
                                                 : u32_text.length();
                        if (global_line_end > global_line_start && u32_text[global_line_end - 1] == '\n') {
                            global_line_end--;
                        }
                        std::u32string line = u32_text.substr(global_line_start, global_line_end - global_line_start);
                        m_highlight_cache[line_number] = m_highlighter->highlight_line(line);
                        m_highlight_cache_valid[line_number] = true;
                    }
                    
                    const auto& tokens = m_highlight_cache[line_number];
                    for (const auto& token : tokens) {
                        size_t s = line_start_char + token.start;
                        size_t e = line_start_char + token.end;
                        if (s < local_byte_offsets.size() && e < local_byte_offsets.size()) {
                            PangoAttribute* attr = pango_attr_foreground_new(
                                SyntaxHighlighter::get_token_color(token.type).r * 65535,
                                SyntaxHighlighter::get_token_color(token.type).g * 65535,
                                SyntaxHighlighter::get_token_color(token.type).b * 65535);
                            attr->start_index = (int)local_byte_offsets[s];
                            attr->end_index   = (int)local_byte_offsets[e];
                            pango_attr_list_insert(attrs, attr);
                        }
                    }
                }
                line_start_char = i + 1;
                ++line_number;
            }
        }
    }
    // Apply selection foreground attribute
    if (sel_start != sel_end) {
        size_t visible_start_char = line_starts_chars[vis_first_line];
        size_t visible_end_char = (vis_last_line + 1 < (int)line_starts_chars.size()) 
                                  ? line_starts_chars[vis_last_line + 1] 
                                  : m_doc->get_length();

        size_t s_char = std::min(sel_start, sel_end);
        size_t e_char = std::max(sel_start, sel_end);

        if (e_char > visible_start_char && s_char < visible_end_char) {
            size_t s_layout = std::max((size_t)0, s_char - visible_start_char);
            size_t e_layout = std::min(visible_u32.length(), e_char - visible_start_char);

            if (s_layout < e_layout && s_layout < local_byte_offsets.size() && e_layout < local_byte_offsets.size()) {
                auto* tm = theme_manager();
                if (tm) {
                    Color sel_fg = tm->get_color("menu_item_selected_fg");
                    PangoAttribute* attr = pango_attr_foreground_new(
                        sel_fg.r * 65535,
                        sel_fg.g * 65535,
                        sel_fg.b * 65535
                    );
                    attr->start_index = (int)local_byte_offsets[s_layout];
                    attr->end_index   = (int)local_byte_offsets[e_layout];
                    pango_attr_list_change(attrs, attr);
                }
            }
        }
    }

    pango_layout_set_attributes(m_layout, attrs);
    pango_attr_list_unref(attrs);

    last_ver = current_version;
    last_first = vis_first_line;
    last_last = vis_last_line;
    last_sel_start = sel_start;
    last_sel_end = sel_end;
    m_last_layout_version = current_version;
    m_last_scroll_y = current_scroll_y;
    return true;
}

void TextEditorWidget::measure_font_metrics() {
    if (!m_layout || !m_doc) return;
    
    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, m_font_family.c_str());
    pango_font_description_set_absolute_size(desc, m_font_size * PANGO_SCALE);
    pango_font_description_set_weight(desc, m_font_weight == 1 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
    pango_layout_set_font_description(m_layout, desc);
    pango_font_description_free(desc);

    pango_layout_set_text(m_layout, "A", -1);
    int w1 = 0, h1 = 0;
    pango_layout_get_pixel_size(m_layout, &w1, &h1);

    pango_layout_set_text(m_layout, "AA", -1);
    int w2 = 0, h2 = 0;
    pango_layout_get_pixel_size(m_layout, &w2, &h2);

    float char_w = w2 - w1;
    if (char_w < 1.0f) char_w = m_font_size * 0.6f;

    // Reset layout text to "A" to get line height and baseline
    pango_layout_set_text(m_layout, "A", -1);
    PangoLayoutIter* iter = pango_layout_get_iter(m_layout);
    if (iter) {
        PangoRectangle extents;
        pango_layout_iter_get_line_extents(iter, nullptr, &extents);
        float line_h = (float)extents.height / PANGO_SCALE;
        if (line_h < 1.0f) line_h = m_font_size;
        
        float ascent = (float)pango_layout_iter_get_baseline(iter) / PANGO_SCALE;
        
        m_doc->set_metrics(line_h, ascent, char_w);
        pango_layout_iter_free(iter);
    }
}

int TextEditorWidget::get_pixel_x_for_char(int line_num, int col) {
    if (!m_doc || col <= 0) return 0;
    
    std::lock_guard<std::recursive_mutex> lock(m_doc->m_mutex);
    const auto& line_starts_chars = m_doc->ensure_line_index();
    if (line_num >= (int)line_starts_chars.size()) return 0;
    
    const std::u32string& u32_text = m_doc->get_data();
    size_t line_start = line_starts_chars[line_num];
    size_t line_end = (line_num + 1 < (int)line_starts_chars.size()) 
                      ? line_starts_chars[line_num + 1] 
                      : u32_text.length();
    
    if (line_end > line_start && u32_text[line_end - 1] == '\n') {
        line_end--;
    }
    
    size_t col_len = std::min((size_t)col, line_end - line_start);
    if (col_len == 0) return 0;
    
    std::u32string line_part = u32_text.substr(line_start, col_len);
    
    std::string utf8_part;
    utf8_part.reserve(line_part.length() * 2);
    for (char32_t c : line_part) {
        if (c <= 0x7F) utf8_part.push_back((char)c);
        else if (c <= 0x7FF) { utf8_part.push_back((char)(0xC0 | (c >> 6))); utf8_part.push_back((char)(0x80 | (c & 0x3F))); }
        else if (c <= 0xFFFF) { utf8_part.push_back((char)(0xE0 | (c >> 12))); utf8_part.push_back((char)(0x80 | ((c >> 6) & 0x3F))); utf8_part.push_back((char)(0x80 | (c & 0x3F))); }
        else { utf8_part.push_back((char)(0xF0 | (c >> 18))); utf8_part.push_back((char)(0x80 | ((c >> 12) & 0x3F))); utf8_part.push_back((char)(0x80 | ((c >> 6) & 0x3F))); utf8_part.push_back((char)(0x80 | (c & 0x3F))); }
    }
    
    PangoLayout* measure_layout = nullptr;
    if (m_layout) {
        PangoContext* context = pango_layout_get_context(m_layout);
        measure_layout = pango_layout_new(context);
        const PangoFontDescription* desc = pango_layout_get_font_description(m_layout);
        if (desc) {
            pango_layout_set_font_description(measure_layout, desc);
        }
    } else {
        cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* dummy_cr = cairo_create(surface);
        measure_layout = pango_cairo_create_layout(dummy_cr);
        cairo_destroy(dummy_cr);
        cairo_surface_destroy(surface);
        
        PangoFontDescription* desc = pango_font_description_new();
        pango_font_description_set_family(desc, m_font_family.c_str());
        pango_font_description_set_absolute_size(desc, m_font_size * PANGO_SCALE);
        pango_font_description_set_weight(desc, m_font_weight == 1 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
        pango_layout_set_font_description(measure_layout, desc);
        pango_font_description_free(desc);
    }
    
    pango_layout_set_text(measure_layout, utf8_part.c_str(), -1);
    pango_layout_set_attributes(measure_layout, nullptr);
    
    int w = 0, h = 0;
    pango_layout_get_pixel_size(measure_layout, &w, &h);
    
    g_object_unref(measure_layout);
    return w;
}

void TextEditorWidget::ensure_metrics() {
    if (!m_doc) return;
    
    uint64_t current_version = m_doc->get_version();
    
    // Only re-calculate line metrics if they are empty OR document version changed.
    if (m_doc->get_line_metrics().empty() || current_version != m_last_layout_version) {
        measure_font_metrics();
        
        std::vector<TextDocument::LineMetric> metrics;
        const auto& line_starts_chars = m_doc->ensure_line_index();
        
        ensure_byte_offsets();
        std::vector<size_t> logical_line_starts_bytes;
        logical_line_starts_bytes.reserve(line_starts_chars.size());
        for (int char_idx : line_starts_chars) {
            logical_line_starts_bytes.push_back(
                (char_idx < (int)m_byte_offsets.size()) ? m_byte_offsets[char_idx] : m_byte_offsets.back()
            );
        }
        
        float line_h = m_doc->m_line_height;
        metrics.reserve(line_starts_chars.size());
        for (size_t i = 0; i < line_starts_chars.size(); ++i) {
            size_t start_byte = logical_line_starts_bytes[i];
            size_t end_byte = (i + 1 < logical_line_starts_bytes.size()) ? logical_line_starts_bytes[i + 1] : m_byte_offsets.back();
            metrics.push_back({
                (float)(i * line_h), // y_offset
                line_h,              // height
                start_byte,          // start_byte
                end_byte,            // end_byte
                (int)i               // logical_line
            });
        }
        m_doc->set_line_metrics(metrics);
        m_last_layout_version = current_version;
    }
}

int TextEditorWidget::get_char_index_at(double x, double y) {
    if (!m_doc) return 0;
    ensure_metrics();
    
    int margin_x = (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int margin_y = 5;

    double lx = x - m_x - margin_x;
    double ly = y - m_y - margin_y;

    float line_height = m_doc->m_line_height;
    if (line_height < 1.0f) line_height = m_font_size;

    int row = (int)(ly / line_height);
    const auto& line_starts_chars = m_doc->ensure_line_index();
    if (row < 0) row = 0;
    if (row >= (int)line_starts_chars.size()) row = (int)line_starts_chars.size() - 1;

    int low = 0;
    int high = m_doc->get_line_length(row);
    int best_col = 0;
    int best_diff = INT_MAX;
    
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int px = get_pixel_x_for_char(row, mid);
        int diff = std::abs(px - (int)lx);
        if (diff < best_diff) {
            best_diff = diff;
            best_col = mid;
        }
        if (px < lx) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return line_starts_chars[row] + best_col;
}

void TextEditorWidget::handle_mouse_event(MouseButtonEventContext& ev) {
    if (!m_doc) return;
    ensure_metrics();

    int clicked_idx = get_char_index_at(ev.x, ev.y);

    bool should_click = true;
    if (ev.button == BTN_RIGHT) {
        int sel_start = m_doc->get_selection_start();
        int sel_end = m_doc->get_selection_end();
        if (sel_start != sel_end && clicked_idx >= std::min(sel_start, sel_end) && clicked_idx <= std::max(sel_start, sel_end)) {
            should_click = false;
        }
    }

    if (should_click) {
        if (ev.button == BTN_LEFT || ev.button == BTN_RIGHT) {
            m_doc->set_cursor_at_index(clicked_idx, false);
        }
    }

    set_focus(true);
    m_needs_ensure_visible = true;
    m_cursor_visible = true;
    m_last_blink = std::chrono::steady_clock::now();

    EventContext cursor_ctx;
    cursor_ctx.sender = this;
    when_cursor_moved.run(cursor_ctx);
    invalidate();
}

void TextEditorWidget::handle_mouse_drag(MouseMoveEventContext& ev) {
    if (!m_doc) return;
    ensure_metrics();
    int clicked_idx = get_char_index_at(ev.x, ev.y);
    m_doc->set_cursor_at_index(clicked_idx, true);
    m_needs_ensure_visible = true;
    EventContext cursor_ctx;
    cursor_ctx.sender = this;
    when_cursor_moved.run(cursor_ctx);
    invalidate();
}

int TextEditorWidget::preferred_width() const {
    return 800; // Sensible default
}

int TextEditorWidget::preferred_height() const {
    return 600;
}

bool TextEditorWidget::is_content_modified() const {
    return m_doc ? m_doc->is_dirty() : Widget::is_content_modified();
}

bool TextEditorWidget::can_perform(ClipboardAction action) const {
    if (!m_doc) return false;
    switch (action) {
        case ClipboardAction::Copy:
        case ClipboardAction::Cut:
            return m_doc->get_selection_start() != m_doc->get_selection_end();
        case ClipboardAction::Paste:
            return true;
    }
    return false;
}

void TextEditorWidget::perform(ClipboardAction action) {
    if (!m_doc) return;
    switch (action) {
        case ClipboardAction::Copy:
            m_clipboard_buffer = m_doc->get_selected_text();
            if (!m_clipboard_buffer.empty()) {
                application()->set_clipboard_owner(this);
            }
            break;
        case ClipboardAction::Cut:
            m_clipboard_buffer = m_doc->get_selected_text();
            if (!m_clipboard_buffer.empty()) {
                application()->set_clipboard_owner(this);
                m_doc->handle_key((int)EditorKey::Delete);
            }
            break;
        case ClipboardAction::Paste:
            application()->request_clipboard_data(this);
            break;
    }
}

void TextEditorWidget::provide_clipboard_data(const std::string& mime, DataSink& sink) {
    if (mime == "text/plain" || mime == "text/plain;charset=utf-8") {
        if (!m_clipboard_buffer.empty()) {
            sink.write(std::vector<uint8_t>(m_clipboard_buffer.begin(), m_clipboard_buffer.end()));
            sink.done();
        } else {
            sink.error();
        }
    } else {
        sink.error();
    }
}

void TextEditorWidget::on_clipboard_data_received(const std::string& mime, const std::vector<uint8_t>& data) {
    if (mime == "text/plain" || mime == "text/plain;charset=utf-8") {
        std::string text(data.begin(), data.end());
        m_doc->insert_text(text);
        invalidate();
    }
}

void TextEditorWidget::ensure_cursor_visible() {
    if (!m_doc) return;

    int cursor_row, cursor_col;
    m_doc->get_cursor_row_col(cursor_row, cursor_col);
    int cursor_row_0 = cursor_row - 1;
    int cursor_col_0 = cursor_col - 1;
    float line_height = m_doc->m_line_height;
    if (line_height < 1.0f) line_height = m_font_size;

    int cx = get_pixel_x_for_char(cursor_row_0, cursor_col_0);
    int cy = cursor_row_0 * line_height;
    int cw = 2;
    int ch = line_height;

    int text_x_offset = (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    cx += text_x_offset;

    auto* scroll = dynamic_cast<ScrollArea*>(m_parent);
    if (scroll) {
        int view_w = scroll->width();
        int view_h = scroll->height();
        int cur_sx_val = scroll->scroll_x();
        int cur_sy = scroll->scroll_y();

        int new_sx = cur_sx_val;
        int new_sy = cur_sy;

        if (cx < cur_sx_val + 20) {
            new_sx = std::max(0, cx - 20);
        } else if (cx + cw > cur_sx_val + view_w - 20) {
            new_sx = cx + cw - view_w + 20;
        }

        if (cy < cur_sy + 20) {
            new_sy = std::max(0, (int)cy - 20);
        } else if (cy + ch > cur_sy + view_h - 20) {
            new_sy = (int)(cy + ch - view_h + 20);
        }

        if (new_sx != cur_sx_val || new_sy != cur_sy) {
            scroll->set_scroll_position(new_sx, new_sy);
        }
    }
}

void TextEditorWidget::invalidate_layout() {
    m_last_layout_version = 0xFFFFFFFFFFFFFFFF;
    if (m_doc) {
        m_doc->set_line_metrics({});
    }
}

void TextEditorWidget::set_font_family(const std::string& family) {
    if (m_font_family == family) return;
    m_font_family = family;
    invalidate_layout();
    invalidate();
}

void TextEditorWidget::set_font_size(double size) {
    if (m_font_size == size) return;
    m_font_size = size;
    invalidate_layout();
    invalidate();
}

void TextEditorWidget::set_font_weight(int weight) {
    if (m_font_weight == weight) return;
    m_font_weight = weight;
    invalidate_layout();
    invalidate();
}

void TextEditorWidget::set_show_line_numbers(bool show) {
    m_show_line_numbers = show;
    invalidate();
}

void TextEditorWidget::set_highlight_current_line(bool highlight) {
    m_highlight_current_line = highlight;
    invalidate();
}

static cairo_status_t cairo_pdf_write_func(void *closure, const unsigned char *data, unsigned int length) {
    auto* vec = static_cast<std::vector<uint8_t>*>(closure);
    vec->insert(vec->end(), data, data + length);
    return CAIRO_STATUS_SUCCESS;
}

horizon::print::PrintDocument TextEditorWidget::generate_print_document(const horizon::print::PrintConfig& config) {
    horizon::print::PrintDocument doc;
    doc.mime_type = "application/pdf";
    doc.filename = "document.pdf";
    doc.title = "Text Document";

    std::vector<uint8_t> pdf_data;

    // Parse paper size to points
    double pt_width = 595.0;  // A4 default
    double pt_height = 842.0;

    if (config.paper_size == "Letter") {
        pt_width = 612.0; pt_height = 792.0;
    } else if (config.paper_size == "Legal") {
        pt_width = 612.0; pt_height = 1008.0;
    } else if (config.paper_size == "Executive") {
        pt_width = 522.0; pt_height = 756.0;
    } else if (config.paper_width_mm > 0 && config.paper_height_mm > 0) {
        pt_width = config.paper_width_mm * 72.0 / 25.4;
        pt_height = config.paper_height_mm * 72.0 / 25.4;
    }

    if (config.orientation == horizon::print::Orientation::Landscape) {
        std::swap(pt_width, pt_height);
    }

    double margin = 36.0; // half inch

    cairo_surface_t* surface = cairo_pdf_surface_create_for_stream(cairo_pdf_write_func, &pdf_data, pt_width, pt_height);
    cairo_t* cr = cairo_create(surface);

    cairo_set_source_rgb(cr, 0, 0, 0); // black text

    PangoLayout* print_layout = pango_cairo_create_layout(cr);
    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, m_font_family.c_str());
    pango_font_description_set_size(desc, m_font_size * PANGO_SCALE);
    pango_font_description_set_weight(desc, static_cast<PangoWeight>(m_font_weight == 0 ? PANGO_WEIGHT_NORMAL : m_font_weight));
    pango_layout_set_font_description(print_layout, desc);
    pango_font_description_free(desc);

    pango_layout_set_width(print_layout, (pt_width - 2 * margin) * PANGO_SCALE);
    pango_layout_set_wrap(print_layout, PANGO_WRAP_WORD_CHAR);
    
    if (m_doc) {
        std::string full_text = m_doc->get_text();
        pango_layout_set_text(print_layout, full_text.c_str(), -1);
    }

    PangoLayoutIter* iter = pango_layout_get_iter(print_layout);
    
    double current_y = margin;
    
    do {
        PangoRectangle extents;
        pango_layout_iter_get_line_extents(iter, nullptr, &extents);
        
        double line_height = PANGO_PIXELS(extents.height) + m_line_spacing;
        
        if (current_y + line_height > pt_height - margin) {
            cairo_show_page(cr);
            current_y = margin;
        }
        
        PangoLayoutLine* line = pango_layout_iter_get_line_readonly(iter);
        
        cairo_move_to(cr, margin, current_y);
        pango_cairo_show_layout_line(cr, line);
        
        current_y += line_height;
        
    } while (pango_layout_iter_next_line(iter));
    
    pango_layout_iter_free(iter);
    g_object_unref(print_layout);
    
    cairo_destroy(cr);
    cairo_surface_finish(surface);
    cairo_surface_destroy(surface);

    doc.data = pdf_data;
    return doc;
}

} // namespace text
} // namespace horizon
