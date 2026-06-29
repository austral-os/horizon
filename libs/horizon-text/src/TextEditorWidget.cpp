#include <horizon/text/TextEditorWidget.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Application.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Logger.hpp>
#include <linux/input-event-codes.h>
#include <algorithm>
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
            this->calculate_layout();
            this->invalidate();
        };
        ensure_metrics();
        calculate_layout();
    }
    invalidate();
}

void TextEditorWidget::draw(GraphicsContext& gc) {
    if (!m_doc) return;
    
    cairo_t* cr = (cairo_t*)gc.getNativeContext();
    if (!cr) return;

    if (!application()) return;
    auto* tm = theme_manager();

    // 1. Snapshot: Acquire lock and copy all necessary data for consistent drawing
    std::u32string u32_text;
    int cursor_pos = 0;
    int sel_start = 0;
    int sel_end = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(m_doc->m_mutex);
        u32_text = m_doc->get_data();
        cursor_pos = m_doc->get_cursor_pos();
        sel_start = m_doc->get_selection_start();
        sel_end = m_doc->get_selection_end();
    }

    // 1. Calculate cursor visibility based on time (blink)
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_blink).count();
    if (elapsed >= 500) {
        m_cursor_visible = !m_cursor_visible;
        m_last_blink = now;
    }
    
    // 2. Clear background
    Color bg_color = tm->get_color("textbox_bg");
    gc.setColor(bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    gc.fillRect(m_x, m_y, m_width, m_height);

    // 3. Setup Pango Layout via unified helper
    ensure_metrics(); // This calls update_pango_layout and syncs line metrics
    if (!m_layout) return;
    pango_cairo_update_layout(cr, m_layout);

    // Generate byte offsets from snapshot (needed for cursor/selection)
    std::vector<size_t> byte_offsets;
    byte_offsets.reserve(u32_text.length() + 1);
    size_t current_byte = 0;
    for (char32_t c : u32_text) {
        byte_offsets.push_back(current_byte);
        if (c <= 0x7F) current_byte += 1;
        else if (c <= 0x7FF) current_byte += 2;
        else if (c <= 0xFFFF) current_byte += 3;
        else current_byte += 4;
    }
    byte_offsets.push_back(current_byte);

    // Calculate metrics for drawing
    int margin_x = (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int margin_y = 5;
    int tx = m_x + margin_x;
    int ty = m_y + margin_y;

    // Get cursor position in pixels
    int cursor_byte = (cursor_pos < (int)byte_offsets.size()) ? (int)byte_offsets[cursor_pos] : (int)byte_offsets.back();
    PangoRectangle cursor_rect;
    pango_layout_get_cursor_pos(m_layout, cursor_byte, &cursor_rect, nullptr);
    int cursor_pixel_x = PANGO_PIXELS(cursor_rect.x);
    int cursor_pixel_y = PANGO_PIXELS(cursor_rect.y);
    int cursor_pixel_h = PANGO_PIXELS(cursor_rect.height);

    if (m_highlight_current_line) {
        Color line_color = tm->get_variant() == "light" ? bg_color.darker(8.0f) : bg_color.lighter(8.0f);
        line_color = line_color.with_alpha(0.85f);
        cairo_set_source_rgba(cr, line_color.r, line_color.g, line_color.b, line_color.a);
        cairo_rectangle(cr, tx, ty + cursor_pixel_y, std::max(0, m_width - margin_x), cursor_pixel_h);
        cairo_fill(cr);
    }

    // 5. Draw Selection Highlight (Behind text)
    if (sel_start != sel_end) {
        int s_idx = std::max(0, std::min((int)u32_text.length(), std::min(sel_start, sel_end)));
        int e_idx = std::max(0, std::min((int)u32_text.length(), std::max(sel_start, sel_end)));
        int start_byte = (int)byte_offsets[s_idx];
        int end_byte = (int)byte_offsets[e_idx];

        cairo_set_source_rgba(cr, 0.7, 0.8, 1.0, 0.3);
        const auto& metrics = m_doc->get_line_metrics();
        int n_lines = pango_layout_get_line_count(m_layout);

        for (int i = 0; i < (int)metrics.size(); ++i) {
            const auto& line_metric = metrics[i];
            size_t line_start = line_metric.start_byte;
            size_t line_end = line_metric.end_byte;

            if (line_end > (size_t)start_byte && line_start < (size_t)end_byte) {
                int r_start = std::max((int)line_start, start_byte);
                int r_end = std::min((int)line_end, end_byte);
                
                PangoRectangle start_rect, end_rect;
                pango_layout_get_cursor_pos(m_layout, r_start, &start_rect, nullptr);
                pango_layout_get_cursor_pos(m_layout, r_end, &end_rect, nullptr);
                
                int ly = ty + (int)line_metric.y_offset;
                int lh = (int)line_metric.height;
                
                int x1 = PANGO_PIXELS(start_rect.x);
                int x2 = PANGO_PIXELS(end_rect.x);
                
                // Handle full line selection
                if (r_start == (int)line_start && r_end >= (int)line_end) {
                    PangoLayoutLine* line = pango_layout_get_line_readonly(m_layout, i);
                    if (line) {
                        PangoRectangle line_ext;
                        pango_layout_line_get_pixel_extents(line, nullptr, &line_ext);
                        x1 = line_ext.x;
                        x2 = line_ext.x + line_ext.width;
                    }
                }

                int rx = tx + std::min(x1, x2);
                int rw = std::abs(x1 - x2);
                if (rw < 5 && r_end >= (int)line_end) rw = 10;

                cairo_rectangle(cr, rx, ly, rw, lh);
                cairo_fill(cr);
            }
        }
    }

    // 6. Draw the actual text
    Color fg_color = tm->get_color("window_fg");
    cairo_set_source_rgba(cr, fg_color.r, fg_color.g, fg_color.b, fg_color.a);
    cairo_move_to(cr, tx, ty);
    pango_cairo_show_layout(cr, m_layout);

    // 6. Draw Margin Background & Line Numbers
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

        // Draw Line Numbers
        PangoLayout* num_layout = pango_cairo_create_layout(cr);
        PangoFontDescription* num_desc = pango_font_description_from_string("sans 8");
        pango_layout_set_font_description(num_layout, num_desc);
        pango_font_description_free(num_desc);

        PangoLayoutIter* num_iter = pango_layout_get_iter(m_layout);
        std::vector<int> logical_line_starts;
        logical_line_starts.push_back(0);
        for (size_t i = 0; i < u32_text.length(); ++i) {
            if (u32_text[i] == '\n') {
                // Find byte offset of next char
                int next_byte = 0;
                for (size_t j = 0; j <= i; ++j) {
                    char32_t c = u32_text[j];
                    if (c <= 0x7F) next_byte += 1;
                    else if (c <= 0x7FF) next_byte += 2;
                    else if (c <= 0xFFFF) next_byte += 3;
                    else next_byte += 4;
                }
                logical_line_starts.push_back(next_byte);
            }
        }

        int current_log_idx = 0;
        do {
            int line_start_byte = pango_layout_iter_get_index(num_iter);
            while (current_log_idx + 1 < (int)logical_line_starts.size() && line_start_byte >= logical_line_starts[current_log_idx + 1]) {
                current_log_idx++;
            }
            
            if (current_log_idx < (int)logical_line_starts.size() && line_start_byte == logical_line_starts[current_log_idx]) {
                PangoRectangle line_rect;
                pango_layout_iter_get_line_extents(num_iter, nullptr, &line_rect);
                int ly = ty + PANGO_PIXELS(line_rect.y);
                
                std::string num_str = std::to_string(current_log_idx + 1);
                pango_layout_set_text(num_layout, num_str.c_str(), -1);
                int nw, nh;
                pango_layout_get_pixel_size(num_layout, &nw, &nh);
                Color num_fg = fg_color.with_alpha(0.5);
                cairo_set_source_rgba(cr, num_fg.r, num_fg.g, num_fg.b, num_fg.a);
                cairo_move_to(cr, m_x + m_line_number_margin - nw - 5, ly + (PANGO_PIXELS(line_rect.height) - nh) / 2);
                pango_cairo_show_layout(cr, num_layout);
            }
        } while (pango_layout_iter_next_line(num_iter));

        pango_layout_iter_free(num_iter);
        g_object_unref(num_layout);
    }


    // 6. Draw Cursor
    if (has_focus() && m_cursor_visible) {
        Color cursor_color = tm->get_color("window_fg");
        cairo_set_source_rgba(cr, cursor_color.r, cursor_color.g, cursor_color.b, cursor_color.a);
        cairo_rectangle(cr, tx + cursor_pixel_x, ty + cursor_pixel_y, 2, cursor_pixel_h);
        cairo_fill(cr);
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

    int text_w = 0, text_h = 0;
    ensure_metrics();

    if (m_layout) {
        pango_layout_get_pixel_size(m_layout, &text_w, &text_h);
    } else {
        text_w = 800;
        text_h = 600;
    }

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

bool TextEditorWidget::update_pango_layout(cairo_t* cr) {
    if (!m_layout) {
        if (cr) {
            m_layout = pango_cairo_create_layout(cr);
        } else {
            // Create a dummy context if we need a layout but don't have a surface yet
            cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
            cairo_t* dummy_cr = cairo_create(surface);
            m_layout = pango_cairo_create_layout(dummy_cr);
            cairo_destroy(dummy_cr);
            cairo_surface_destroy(surface);
        }
    }
    if (!m_layout || !m_doc) return false;

    // Caching: Only update if document version changed
    uint64_t current_version = m_doc->get_version();
    if (current_version == m_last_layout_version) return false;

    // 1. Font and Geometry
    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, m_font_family.c_str());
    pango_font_description_set_absolute_size(desc, m_font_size * PANGO_SCALE);
    pango_font_description_set_weight(desc, m_font_weight == 1 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
    pango_layout_set_font_description(m_layout, desc);
    pango_font_description_free(desc);
    
    pango_layout_set_width(m_layout, -1);
    pango_layout_set_wrap(m_layout, PANGO_WRAP_WORD_CHAR);

    // 2. Text and Attributes from safe snapshot
    std::u32string u32_text;
    {
        std::lock_guard<std::recursive_mutex> lock(m_doc->m_mutex);
        u32_text = m_doc->get_data();
    }

    std::string utf8_text;
    std::vector<size_t> byte_offsets;
    utf8_text.reserve(u32_text.length() * 2);
    byte_offsets.reserve(u32_text.length() + 1);
    
    size_t current_byte = 0;
    for (char32_t c : u32_text) {
        byte_offsets.push_back(current_byte);
        if (c <= 0x7F) { utf8_text.push_back((char)c); current_byte += 1; }
        else if (c <= 0x7FF) { utf8_text.push_back((char)(0xC0 | (c >> 6))); utf8_text.push_back((char)(0x80 | (c & 0x3F))); current_byte += 2; }
        else if (c <= 0xFFFF) { utf8_text.push_back((char)(0xE0 | (c >> 12))); utf8_text.push_back((char)(0x80 | ((c >> 6) & 0x3F))); utf8_text.push_back((char)(0x80 | (c & 0x3F))); current_byte += 3; }
        else { utf8_text.push_back((char)(0xF0 | (c >> 18))); utf8_text.push_back((char)(0x80 | ((c >> 12) & 0x3F))); utf8_text.push_back((char)(0x80 | ((c >> 6) & 0x3F))); utf8_text.push_back((char)(0x80 | (c & 0x3F))); current_byte += 4; }
    }
    byte_offsets.push_back(current_byte);
    pango_layout_set_text(m_layout, utf8_text.c_str(), -1);

    PangoAttrList* attrs = pango_attr_list_new();
    size_t line_start_char = 0;
    for (size_t i = 0; i <= u32_text.length(); ++i) {
        if (i == u32_text.length() || u32_text[i] == '\n') {
            std::u32string line = u32_text.substr(line_start_char, i - line_start_char);
            auto tokens = m_highlighter->highlight_line(line);
            for (const auto& token : tokens) {
                size_t s = line_start_char + token.start;
                size_t e = line_start_char + token.end;
                if (s < byte_offsets.size() && e < byte_offsets.size()) {
                    PangoAttribute* attr = pango_attr_foreground_new(
                        SyntaxHighlighter::get_token_color(token.type).r * 65535,
                        SyntaxHighlighter::get_token_color(token.type).g * 65535,
                        SyntaxHighlighter::get_token_color(token.type).b * 65535);
                    attr->start_index = (int)byte_offsets[s];
                    attr->end_index = (int)byte_offsets[e];
                    pango_attr_list_insert(attrs, attr);
                }
            }
            line_start_char = i + 1;
        }
    }
    pango_layout_set_attributes(m_layout, attrs);
    pango_attr_list_unref(attrs);
    
    return true;
}

void TextEditorWidget::ensure_metrics() {
    if (!m_doc) return;
    
    uint64_t current_version = m_doc->get_version();
    bool updated = update_pango_layout(nullptr);
    
    // Only re-calculate line metrics if they are empty OR document version changed
    if (updated || m_doc->get_line_metrics().empty() || current_version != m_last_layout_version) {
        if (!m_layout) return;
        PangoLayoutIter* iter = pango_layout_get_iter(m_layout);
        if (iter) {
            std::vector<TextDocument::LineMetric> metrics;
            PangoRectangle line_extents;
            do {
                pango_layout_iter_get_line_extents(iter, nullptr, &line_extents);
                PangoLayoutLine* line = pango_layout_iter_get_line_readonly(iter);
                float ly = (float)line_extents.y / PANGO_SCALE;
                float lh = (float)line_extents.height / PANGO_SCALE;
                if (lh < 1.0f) lh = m_font_size; 
                metrics.push_back({ly, lh, (size_t)line->start_index, (size_t)(line->start_index + line->length)});
            } while (pango_layout_iter_next_line(iter));
            m_doc->set_line_metrics(metrics);
            
            // Update general metrics for fallback
            if (!metrics.empty()) {
                pango_layout_iter_free(iter);
                iter = pango_layout_get_iter(m_layout);
                float line_h = metrics[0].height;
                float ascent = (float)pango_layout_iter_get_baseline(iter) / PANGO_SCALE;
                PangoContext* context = pango_layout_get_context(m_layout);
                const PangoFontDescription* desc = pango_layout_get_font_description(m_layout);
                PangoFontMetrics* pmetrics = pango_context_get_metrics(context, desc, nullptr);
                if (pmetrics) {
                    float char_w = (float)pango_font_metrics_get_approximate_char_width(pmetrics) / PANGO_SCALE;
                    if (char_w < 1.0f) char_w = m_font_size * 0.6f;
                    pango_font_metrics_unref(pmetrics);
                    m_doc->set_metrics(line_h, ascent, char_w);
                }
            }
            pango_layout_iter_free(iter);
        }
        m_last_layout_version = current_version;
    }
}

int TextEditorWidget::get_char_index_at(double x, double y) {
    if (!m_layout || !m_doc) return 0;
    
    // We must ensure metrics are up to date for accurate hit-testing
    ensure_metrics();

    int margin_x = (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int margin_y = 5;

    double lx = x - m_x - margin_x;
    double ly = y - m_y - margin_y;

    int byte_index, trailing;
    pango_layout_xy_to_index(m_layout, (int)(lx * PANGO_SCALE), (int)(ly * PANGO_SCALE), &byte_index, &trailing);

    // target_byte is where the user actually clicked (accounting for trailing edge)
    int target_byte = byte_index + trailing;

    // Convert Pango byte index back to UTF-32 index using a safe snapshot
    std::u32string u32_text;
    {
        std::lock_guard<std::recursive_mutex> lock(m_doc->m_mutex);
        u32_text = m_doc->get_data();
    }

    int char_idx = 0;
    int current_byte = 0;
    for (char32_t c : u32_text) {
        if (current_byte >= target_byte) break;
        
        // Special case: if we are at a newline and the user clicked beyond it, 
        // we stop BEFORE the newline to keep the cursor on the same line.
        if (c == '\n' && current_byte + 1 > byte_index) break;

        char_idx++;
        if (c <= 0x7F) current_byte += 1;
        else if (c <= 0x7FF) current_byte += 2;
        else if (c <= 0xFFFF) current_byte += 3;
        else current_byte += 4;
    }

    return char_idx;
}

void TextEditorWidget::handle_mouse_event(MouseButtonEventContext& ev) {
    if (!m_doc) return;
    ensure_metrics();

    int clicked_idx = get_char_index_at(ev.x, ev.y);

    bool should_click = true;
    if (ev.button == BTN_RIGHT) {
        int sel_start = m_doc->get_selection_start();
        int sel_end = m_doc->get_selection_end();
        // If right-click is inside current selection, don't move cursor
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
    // If we have a layout, we could return its height
    // But for a scrollable area, 600 is fine.
    return 600;
}

    bool TextEditorWidget::is_content_modified() const
    {
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
    if (!m_doc || !m_layout) return;

    // 1. Calculate cursor position
    const std::u32string& data = m_doc->get_data();
    int cursor_pos = m_doc->get_cursor_pos();
    int byte_pos = 0;
    for (int i = 0; i < cursor_pos && i < (int)data.length(); ++i) {
        char32_t c = data[i];
        if (c <= 0x7F) byte_pos += 1;
        else if (c <= 0x7FF) byte_pos += 2;
        else if (c <= 0xFFFF) byte_pos += 3;
        else byte_pos += 4;
    }

    PangoRectangle strong_pos, weak_pos;
    pango_layout_get_cursor_pos(m_layout, byte_pos, &strong_pos, &weak_pos);

    int text_x_offset = (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int text_y_offset = 0;

    int cx = text_x_offset + PANGO_PIXELS(strong_pos.x);
    int cy = text_y_offset + PANGO_PIXELS(strong_pos.y);
    int cw = 2; // Cursor width
    int ch = PANGO_PIXELS(strong_pos.height);

    // 2. Find parent ScrollArea and adjust scroll
    auto* scroll = dynamic_cast<ScrollArea*>(m_parent);
    if (scroll) {
        int view_w = scroll->width();
        int view_h = scroll->height();
        int cur_sx = scroll->scroll_x();
        int cur_sy = scroll->scroll_y();

        int new_sx = cur_sx;
        int new_sy = cur_sy;

        // Horizontal scroll adjustment
        if (cx < cur_sx + 20) {
            new_sx = std::max(0, cx - 20);
        } else if (cx + cw > cur_sx + view_w - 20) {
            new_sx = cx + cw - view_w + 20;
        }

        // Vertical scroll adjustment
        if (cy < cur_sy + 20) {
            new_sy = std::max(0, cy - 20);
        } else if (cy + ch > cur_sy + view_h - 20) {
            new_sy = cy + ch - view_h + 20;
        }

        if (new_sx != cur_sx || new_sy != cur_sy) {
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
