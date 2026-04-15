#include <horizon/text/TextEditorWidget.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Application.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Logger.hpp>
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
    
    PangoFontDescription* desc = pango_font_description_from_string("Monospace 12");
    pango_layout_set_font_description(m_layout, desc);
    pango_font_description_free(desc);

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
    int text_y = m_y + 5;

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

    cairo_save(cr);
    cairo_rectangle(cr, text_x, m_y, m_width - (text_x - m_x), m_height);
    cairo_clip(cr);

    // 5. Highlight Current Line
    if (m_highlight_current_line && has_focus()) {
        gc.setColor(0.9, 0.95, 1.0, 1.0); // Subtle light blue
        gc.fillRect(m_x + (m_show_line_numbers ? m_line_number_margin : 0), 
                    cursor_pixel_y, 
                    m_width, 
                    cursor_pixel_h);
    }

    gc.setColor(0.1, 0.1, 0.1, 1.0); // Default color
    cairo_move_to(cr, text_x, text_y);
    pango_cairo_show_layout(cr, m_layout);

    // 6. Draw Selection & Cursor
    if (has_focus()) {
        if (m_doc->get_selection_start() != m_doc->get_selection_end()) {
            // Selection drawing logic
        }

        if (m_cursor_visible) {
            int cx = text_x + PANGO_PIXELS(cursor_strong_pos.x);
            gc.setColor(0.1, 0.1, 0.1, 1.0);
            gc.fillRect(cx, cursor_pixel_y, 2, cursor_pixel_h);
        }
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

void TextEditorWidget::handle_mouse_event(MouseButtonEventContext& ev) {
    if (!m_doc) return;

    int text_x = m_x + (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int text_y = m_y + 5;

    // Map click to char index
    // This requires pango_layout_xy_to_index
    // Simplified for now
    m_doc->handle_click(ev.x - text_x, ev.y - text_y);
    set_focus(true);
    m_needs_ensure_visible = true;
    EventContext cursor_ctx;
    cursor_ctx.sender = this;
    when_cursor_moved.run(cursor_ctx);
    invalidate();
}

void TextEditorWidget::handle_mouse_drag(MouseMoveEventContext& ev) {
    if (!m_doc) return;
    int text_x = m_x + (m_show_line_numbers ? m_line_number_margin + 5 : 5);
    int text_y = m_y + 5;
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
    int text_y_offset = 5;

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
