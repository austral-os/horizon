#pragma once

#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/text/TextDocument.hpp>
#include <horizon/text/SyntaxHighlighter.hpp>
#include <pango/pangocairo.h>
#include <memory>

namespace horizon {
namespace text {

/**
 * @class TextEditorWidget
 * @brief The visual component for editing text.
 */
class TextEditorWidget : public Widget {
public:
    TextEditorWidget();
    virtual ~TextEditorWidget();

    void set_document(std::shared_ptr<TextDocument> doc);
    std::shared_ptr<TextDocument> get_document() const { return m_doc; }

    void draw(GraphicsContext& gc) override;
    void calculate_layout() override;

    EventsManager<EventContext> when_cursor_moved;
    
    int preferred_width() const override;
    int preferred_height() const override;
    
    // Core properties
    void set_font_family(const std::string& family);
    void set_font_size(double size);
    void set_font_weight(int weight);
    void set_line_spacing(double spacing);
    void set_show_line_numbers(bool show);
    void set_highlight_current_line(bool highlight);
    void set_insert_spaces_for_tab(bool insert_spaces);
    void set_spaces_per_tab(int spaces);

    // Feature support
    bool supports_fullscreen() const override { return true; }
    bool supports_clipboard() const override { return true; }
    bool supports_undo() const override { return true; }
    bool supports_printing() const override { return true; }
    bool supports_save_check() const override { return true; }
    bool is_content_modified() const override;
    horizon::print::PrintDocument generate_print_document(const horizon::print::PrintConfig& config) override;

    bool can_perform(ClipboardAction action) const override;
    void perform(ClipboardAction action) override;

    // Clipboard data management
    void provide_clipboard_data(const std::string& mime, DataSink& sink) override;
    void on_clipboard_data_received(const std::string& mime, const std::vector<uint8_t>& data) override;
    std::vector<std::string> provided_mime_types() const override { return {"text/plain", "text/plain;charset=utf-8"}; }
    std::vector<std::string> accepted_mime_types() const override { return {"text/plain", "text/plain;charset=utf-8"}; }

    // Event overrides
    void handle_key_event(KeyEventContext& ev);
    void handle_mouse_event(MouseButtonEventContext& ev);
    void handle_mouse_drag(MouseMoveEventContext& ev);

protected:
    bool update_pango_layout(cairo_t* cr);
    void ensure_metrics();
    int get_char_index_at(double x, double y);
    void ensure_cursor_visible();
    void invalidate_layout();

private:
    std::shared_ptr<TextDocument> m_doc;
    std::unique_ptr<SyntaxHighlighter> m_highlighter;
    PangoLayout* m_layout = nullptr;
    std::string m_font_family = "Monospace";
    double m_font_size = 12.0;
    int m_font_weight = 0;
    double m_line_spacing = 4.0;
    bool m_show_line_numbers = true;
    bool m_highlight_current_line = false;
    bool m_insert_spaces_for_tab = false;
    int m_spaces_per_tab = 4;
    int m_line_number_margin = 40;
    
    // Scroll state (if not using ScrollArea, but we'll try to integrate with it)
    double m_scroll_x = 0;
    double m_scroll_y = 0;
    
    // Internal state
    std::chrono::steady_clock::time_point m_last_blink;
    bool m_cursor_visible = true;
    bool m_needs_ensure_visible = false;
    bool m_metrics_dirty = true;       // true = need to rebuild line metrics
    std::string m_clipboard_buffer;
    uint64_t m_last_layout_version = 0xFFFFFFFFFFFFFFFF;

    // Cached byte-offset table: maps UTF-32 char index -> UTF-8 byte offset.
    // Rebuilt lazily when m_byte_offsets_version != doc version.
    std::vector<size_t> m_byte_offsets;
    uint64_t m_byte_offsets_version = 0xFFFFFFFFFFFFFFFF;

    void ensure_byte_offsets();

    // Syntax highlighting cache: maps logical line number -> list of highlighted tokens
    std::vector<std::vector<SyntaxHighlighter::HighlightedToken>> m_highlight_cache;
    std::vector<bool> m_highlight_cache_valid;
    uint64_t m_highlight_cache_version = 0xFFFFFFFFFFFFFFFF;
    int m_last_scroll_y = -1;
    int m_vis_first_line = 0;
    int m_vis_last_line = 0;

    void ensure_highlight_cache();
    void measure_font_metrics();
    int get_pixel_x_for_char(int line_num, int col);
};

} // namespace text
} // namespace horizon
