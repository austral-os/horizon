#pragma once

#include <horizon/Widget.hpp>
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
    
    int preferred_width() const override;
    int preferred_height() const override;
    
    // Core properties
    void set_font_size(double size);
    void set_line_spacing(double spacing);
    void set_show_line_numbers(bool show);

    // Event overrides
    void handle_key_event(KeyEventContext& ev);
    void handle_mouse_event(MouseButtonEventContext& ev);
    void handle_mouse_drag(MouseMoveEventContext& ev);

protected:
    void update_pango_layout(cairo_t* cr);
    int get_char_index_at(double x, double y);
    void ensure_cursor_visible();

private:
    std::shared_ptr<TextDocument> m_doc;
    std::unique_ptr<SyntaxHighlighter> m_highlighter;
    PangoLayout* m_layout = nullptr;
    double m_font_size = 12.0;
    double m_line_spacing = 4.0;
    bool m_show_line_numbers = true;
    int m_line_number_margin = 40;
    
    // Scroll state (if not using ScrollArea, but we'll try to integrate with it)
    double m_scroll_x = 0;
    double m_scroll_y = 0;
    
    // Internal state
    std::chrono::steady_clock::time_point m_last_blink;
    bool m_cursor_visible = true;
};

} // namespace text
} // namespace horizon
