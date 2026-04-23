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
    
    when_application_load.connect([this](EventContext&) {
        if (application()) {
            application()->add_timer(500, [this]() {
                if (this->has_focus()) {
                    this->m_cursor_visible = !this->m_cursor_visible;
                    this->invalidate();
                }
            }, true);
        }
    });

    when_focus.connect([this](EventContext&) {
        m_cursor_visible = true;
        m_last_blink = std::chrono::steady_clock::now();
        invalidate();
    });
}

TextEditorWidget::~TextEditorWidget() {
    if (m_layout) g_object_unref(m_layout);
}

void TextEditorWidget::set_document(std::shared_ptr<TextDocument> doc) {
    m_doc = doc;
    if (m_doc) {
        m_doc->on_changed = [this]() {
            this->invalidate();
        };
        set_size(preferred_width(), preferred_height());
    }
    invalidate();
}

void TextEditorWidget::draw(GraphicsContext& gc) {
    if (!m_doc) return;
    
    cairo_t* cr = (cairo_t*)gc.getNativeContext();
    if (!cr) return;

    auto* tm = application()->theme_manager.get();
    auto font = tm->get_font("window");

    // 1. Draw Background
    gc.setColor(1.0, 1.0, 1.0, 1.0);
    gc.fillRect(m_x, m_y, m_width, m_height);

    // 2. Setup Pango
    if (!m_layout) {
        m_layout = pango_cairo_create_layout(cr);
    }
    
    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, m_font_family.c_str());
    pango_font_description_set_absolute_size(desc, m_font_size * PANGO_SCALE);
    if (m_font_weight == 1) {
        pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
    } else {
        pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
    }
    pango_layout_set_font_description(m_layout, desc);
    pango_font_description_free(desc);

    ensure_metrics();

    // 3. Draw Line Numbers Margin
    if (m_show_line_numbers) {
        gc.setColor(0.95, 0.95, 0.95, 1.0);
        gc.fillRect(m_x, m_y, m_line_number_margin, m_height);
        gc.setColor(0.8, 0.8, 0.8, 1.0);
        gc.drawLine(m_x + m_line_number_margin, m_y, m_x + m_line_number_margin, m_y + m_height);
    }

    // 4. Draw Text with Highlighting
    std::string utf8_text = m_doc->get_text();
    pango_layout_set_text(m_layout, utf8_text.c_str(), -1);

    // Apply Highlighting
    PangoAttrList* attrs = pango_attr_list_new();
    
    // We'll highlight line by line or the whole text?
    // Let's do it for the whole text for now as it's simpler to implement.
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string u32_text = converter.from_bytes(utf8_text);
    
    // Split into lines and highlight each
    size_t start_of_line = 0;
    size_t char_idx = 0;
    while (char_idx <= u32_text.length()) {
        if (char_idx == u32_text.length() || u32_text[char_idx] == '\n') {
            std::u32string line = u32_text.substr(start_of_line, char_idx - start_of_line);
            auto tokens = m_highlighter->highlight_line(line);
            
            for (const auto& token : tokens) {
                // Map char indices to byte indices for Pango
                size_t start_in_text = start_of_line + token.start;
                size_t end_in_text = start_of_line + token.end;
                
                size_t start_byte = converter.to_bytes(u32_text.substr(0, start_in_text)).length();
                size_t end_byte = converter.to_bytes(u32_text.substr(0, end_in_text)).length();
                
                Color c = SyntaxHighlighter::get_token_color(token.type);
                PangoAttribute* attr = pango_attr_foreground_new(c.r * 65535, c.g * 65535, c.b * 65535);
                attr->start_index = start_byte;
                attr->end_index = end_byte;
                pango_attr_list_insert(attrs, attr);
            }
            start_of_line = char_idx + 1;
        }
        char_idx++;
    }
    
    pango_layout_set_attributes(m_layout, attrs);
    pango_attr_list_unref(attrs);

    int text_x = m_x + (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int text_y = m_y;

    // Calculate cursor position in pixels for highlighting and cursor drawing
    int byte_pos = 0;
    {
        const char* start = utf8_text.c_str();
        const char* p = start;
        int cursor_pos = m_doc->get_cursor_pos();
        for (int i = 0; i < cursor_pos && *p; ++i) {
            p = g_utf8_next_char(p);
        }
        byte_pos = p - start;
    }

    PangoRectangle cursor_strong_pos;
    pango_layout_get_cursor_pos(m_layout, byte_pos, &cursor_strong_pos, nullptr);
    int cursor_pixel_y = text_y + PANGO_PIXELS(cursor_strong_pos.y);
    int cursor_pixel_h = PANGO_PIXELS(cursor_strong_pos.height);

    // 4.5. Draw Margin Background
    if (m_show_line_numbers) {
        cairo_save(cr);
        cairo_set_source_rgb(cr, 0.96, 0.96, 0.96);
        cairo_rectangle(cr, m_x, m_y, m_line_number_margin, m_height);
        cairo_fill(cr);
        
        cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
        cairo_move_to(cr, m_x + m_line_number_margin, m_y);
        cairo_line_to(cr, m_x + m_line_number_margin, m_y + m_height);
        cairo_stroke(cr);
        
        // Find logical line starts
        std::vector<int> logical_line_starts;
        logical_line_starts.push_back(0);
        const char* start_ptr = utf8_text.c_str();
        const char* p_ptr = start_ptr;
        while (*p_ptr) {
            if (*p_ptr == '\n') {
                logical_line_starts.push_back((p_ptr - start_ptr) + 1);
            }
            p_ptr++;
        }

        // Draw line numbers
        cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
        PangoLayout* num_layout = pango_cairo_create_layout(cr);
        PangoFontDescription* num_font = pango_font_description_new();
        pango_font_description_set_family(num_font, m_font_family.c_str());
        pango_font_description_set_absolute_size(num_font, m_font_size * 0.9 * PANGO_SCALE);
        pango_layout_set_font_description(num_layout, num_font);
        pango_font_description_free(num_font);

        PangoLayoutIter* num_iter = pango_layout_get_iter(m_layout);
        size_t current_log_idx = 0;
        do {
            PangoLayoutLine* line = pango_layout_iter_get_line_readonly(num_iter);
            int line_start_byte = line->start_index;
            
            if (current_log_idx < logical_line_starts.size() && line_start_byte == logical_line_starts[current_log_idx]) {
                PangoRectangle line_rect;
                pango_layout_iter_get_line_extents(num_iter, nullptr, &line_rect);
                int ly = text_y + PANGO_PIXELS(line_rect.y);
                
                std::string num_str = std::to_string(current_log_idx + 1);
                pango_layout_set_text(num_layout, num_str.c_str(), -1);
                
                int nw, nh;
                pango_layout_get_pixel_size(num_layout, &nw, &nh);
                
                cairo_move_to(cr, m_x + m_line_number_margin - nw - 5, ly);
                pango_cairo_show_layout(cr, num_layout);
                
                current_log_idx++;
            }
        } while (pango_layout_iter_next_line(num_iter));
        pango_layout_iter_free(num_iter);
        g_object_unref(num_layout);
        cairo_restore(cr);
    }

    cairo_save(cr);
    cairo_rectangle(cr, text_x, m_y, m_width - (text_x - m_x), m_height);
    cairo_clip(cr);

    // 5. Highlight Current Line & Selection
    if (has_focus()) {
        if (m_highlight_current_line) {
            gc.setColor(0.9, 0.95, 1.0, 1.0); // Subtle light blue
            gc.fillRect(m_x + (m_show_line_numbers ? m_line_number_margin : 0), 
                        cursor_pixel_y, 
                        m_width, 
                        cursor_pixel_h);
        }

        if (m_doc->get_selection_start() != m_doc->get_selection_end()) {
            int sel_start_char = m_doc->get_selection_start();
            int sel_end_char = m_doc->get_selection_end();

            // Convert character indices to byte indices
            int sel_start_byte = 0;
            int sel_end_byte = 0;
            {
                const char* start = utf8_text.c_str();
                const char* p = start;
                for (int i = 0; i < sel_start_char && *p; ++i) p = g_utf8_next_char(p);
                sel_start_byte = p - start;
                
                p = start;
                for (int i = 0; i < sel_end_char && *p; ++i) p = g_utf8_next_char(p);
                sel_end_byte = p - start;
            }

            // Draw selection background for each line
            gc.setColor(0.3, 0.6, 1.0, 0.4); // Semi-transparent selection blue
            
            PangoLayoutIter* iter = pango_layout_get_iter(m_layout);
            do {
                PangoLayoutLine* line = pango_layout_iter_get_line_readonly(iter);
                int line_start_byte = line->start_index;
                int line_end_byte = line_start_byte + line->length;

                int intersect_start = std::max(sel_start_byte, line_start_byte);
                int intersect_end = std::min(sel_end_byte, line_end_byte);

                if (intersect_start < intersect_end) {
                    int* ranges = nullptr;
                    int n_ranges = 0;
                    pango_layout_line_get_x_ranges(line, intersect_start, intersect_end, &ranges, &n_ranges);

                    PangoRectangle line_rect;
                    pango_layout_iter_get_line_extents(iter, nullptr, &line_rect);
                    int ly = text_y + PANGO_PIXELS(line_rect.y);
                    int lh = PANGO_PIXELS(line_rect.height);

                    for (int i = 0; i < n_ranges; ++i) {
                        int rx = text_x + PANGO_PIXELS(ranges[2*i]);
                        int rw = PANGO_PIXELS(ranges[2*i+1] - ranges[2*i]);
                        gc.fillRect(rx, ly, rw, lh);
                    }
                    g_free(ranges);
                }
            } while (pango_layout_iter_next_line(iter));
            pango_layout_iter_free(iter);
        }
    }

    gc.setColor(0.1, 0.1, 0.1, 1.0); // Default color
    cairo_move_to(cr, text_x, text_y);
    pango_cairo_show_layout(cr, m_layout);

    // 6. Draw Cursor
    if (has_focus() && m_cursor_visible) {
        int cx = text_x + PANGO_PIXELS(cursor_strong_pos.x);
        gc.setColor(0.1, 0.1, 0.1, 1.0);
        gc.fillRect(cx, cursor_pixel_y, 2, cursor_pixel_h);
    }

    cairo_restore(cr);
    
    if (m_needs_ensure_visible && has_focus()) {
        ensure_cursor_visible();
        m_needs_ensure_visible = false;
    }
}

void TextEditorWidget::handle_key_event(KeyEventContext& ev) {
    if (!m_doc) return;

    int key = 0;
    bool shift = ev.modifiers & 0x1;
    bool ctrl = ev.modifiers & 0x4; // Assumption for now

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
        // Regular text input (includes symbols and characters with Shift already applied)
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        try {
            std::u32string u32 = converter.from_bytes(ev.text);
            for (char32_t c : u32) {
                m_doc->handle_key((int)c);
            }
            m_cursor_visible = true;
            m_last_blink = std::chrono::steady_clock::now();
            invalidate();
            ev.stop_propagation = true;
        } catch (...) {
            // Decoding error
        }
    }
    
    m_needs_ensure_visible = true;
    EventContext cursor_ctx;
    cursor_ctx.sender = this;
    when_cursor_moved.run(cursor_ctx);
}

void TextEditorWidget::calculate_layout() {
    if (!m_doc) return;

    int text_w = 0, text_h = 0;
    if (m_layout) {
        pango_layout_get_pixel_size(m_layout, &text_w, &text_h);
    } else {
        // Fallback or initial estimate if layout not created yet
        text_w = 800;
        text_h = 600;
    }

    int margin_x = (m_show_line_numbers ? m_line_number_margin + 20 : 20);
    int margin_y = 20;

    int target_w = text_w + margin_x;
    int target_h = text_h + margin_y;

    if (m_parent) {
        target_w = std::max(target_w, m_parent->width());
        target_h = std::max(target_h, m_parent->height());
    }

    if (target_w != m_width || target_h != m_height) {
        set_size(target_w, target_h);
    }
}

void TextEditorWidget::ensure_metrics() {
    if (!m_layout || !m_doc) return;

    PangoLayoutIter* iter = pango_layout_get_iter(m_layout);
    PangoRectangle line_extents;
    pango_layout_iter_get_line_extents(iter, nullptr, &line_extents);
    float line_h = (float)line_extents.height / PANGO_SCALE;
    float ascent = (float)pango_layout_iter_get_baseline(iter) / PANGO_SCALE;
    pango_layout_iter_free(iter);

    PangoContext* context = pango_layout_get_context(m_layout);
    PangoFontMetrics* pmetrics = pango_context_get_metrics(context, pango_layout_get_font_description(m_layout), nullptr);
    float char_w = (float)pango_font_metrics_get_approximate_char_width(pmetrics) / PANGO_SCALE;
    pango_font_metrics_unref(pmetrics);
    
    m_doc->set_metrics(line_h, ascent, char_w);
}

int TextEditorWidget::get_char_index_at(double x, double y) {
    if (!m_layout || !m_doc) return -1;
    ensure_metrics();

    int index, trailing;
    int text_x = m_x + (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int text_y = m_y;

    pango_layout_xy_to_index(m_layout, (x - text_x) * PANGO_SCALE, (y - text_y) * PANGO_SCALE, &index, &trailing);

    std::string text = m_doc->get_text();
    if (index < 0) return 0;
    if (index >= (int)text.length()) return m_doc->get_length();

    return g_utf8_pointer_to_offset(text.c_str(), text.c_str() + index);
}

void TextEditorWidget::handle_mouse_event(MouseButtonEventContext& ev) {
    if (!m_doc) return;
    ensure_metrics();

    int text_x = m_x + (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int text_y = m_y;

    bool should_click = true;
    if (ev.button == BTN_RIGHT) {
        int clicked_idx = get_char_index_at(ev.x, ev.y);
        int sel_start = m_doc->get_selection_start();
        int sel_end = m_doc->get_selection_end();

        // If right-click is inside current selection, don't move cursor/clear selection
        if (sel_start != sel_end && clicked_idx >= sel_start && clicked_idx <= sel_end) {
            should_click = false;
        }
    }

    if (should_click) {
        m_doc->handle_click(ev.x - text_x, ev.y - text_y);
    }

    set_focus(true);
    m_needs_ensure_visible = true;
    EventContext cursor_ctx;
    cursor_ctx.sender = this;
    when_cursor_moved.run(cursor_ctx);
    invalidate();
}

void TextEditorWidget::handle_mouse_drag(MouseMoveEventContext& ev) {
    if (!m_doc) return;
    ensure_metrics();
    int text_x = m_x + (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int text_y = m_y;
    m_doc->handle_drag(ev.x - text_x, ev.y - text_y);
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
    if (mime == "text/plain") {
        std::string text(data.begin(), data.end());
        m_doc->insert_text(text);
        invalidate();
    }
}

void TextEditorWidget::ensure_cursor_visible() {
    if (!m_doc || !m_layout) return;

    // 1. Calculate cursor position
    std::string utf8_text = m_doc->get_text();
    int byte_pos = 0;
    const char* start = utf8_text.c_str();
    const char* p = start;
    int cursor_pos = m_doc->get_cursor_pos();
    for (int i = 0; i < cursor_pos && *p; ++i) {
        p = g_utf8_next_char(p);
    }
    byte_pos = p - start;

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
void TextEditorWidget::set_font_family(const std::string& family) {
    m_font_family = family;
    invalidate();
}

void TextEditorWidget::set_font_size(double size) {
    m_font_size = size;
    invalidate();
}

void TextEditorWidget::set_font_weight(int weight) {
    m_font_weight = weight;
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

} // namespace text
} // namespace horizon
