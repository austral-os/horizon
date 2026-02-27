#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/TextBox.hpp>
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace horizon
{
    TextBox::TextBox() : Widget()
    {
        set_fixed_size(40);
        set_cursor_type(CursorType::Text);
        set_focusable(true);

        m_last_blink_time = std::chrono::steady_clock::now();

        when_mouse_press.connect(
            [this](EventContext &ev)
            {
                m_selection_anchor = -1; // Reset to force new anchor in draw()
                m_is_dragging = true;
                m_has_pending_click = true;
                m_pending_click_x = (int)ev.eventX;
                invalidate();
            });

        when_mouse_drag.connect(
            [this](EventContext &ev)
            {
                if (m_is_dragging)
                {
                    m_has_pending_click = true;
                    m_pending_click_x = (int)ev.eventX;
                    invalidate();
                }
            });

        when_mouse_release.connect([this](EventContext &ev) { m_is_dragging = false; });

        when_key_press.connect(
            [this](EventContext &ev)
            {
                if (!has_focus())
                    return;

                m_cursor_visible = true;
                m_last_blink_time = std::chrono::steady_clock::now();

                bool shift = ev.modifiers & 0x1; // Application::SHIFT is 1

                auto delete_selection = [this]() -> bool
                {
                    if (m_selection_anchor != -1 && m_selection_anchor != m_cursor_pos)
                    {
                        int start = std::min(m_selection_anchor, m_cursor_pos);
                        int end = std::max(m_selection_anchor, m_cursor_pos);
                        m_text.erase(start, end - start);
                        m_cursor_pos = start;
                        m_selection_anchor = -1;
                        return true;
                    }
                    return false;
                };

                if (ev.keysym == XKB_KEY_BackSpace)
                {
                    if (!delete_selection())
                    {
                        if (m_cursor_pos > 0 && !m_text.empty())
                        {
                            m_text.erase(m_cursor_pos - 1, 1);
                            m_cursor_pos--;
                        }
                    }
                    invalidate();
                }
                else if (ev.keysym == XKB_KEY_Delete)
                {
                    if (!delete_selection())
                    {
                        if (m_cursor_pos < (int)m_text.length())
                        {
                            m_text.erase(m_cursor_pos, 1);
                        }
                    }
                    invalidate();
                }
                else if (ev.keysym == XKB_KEY_Left || ev.keysym == XKB_KEY_KP_Left)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;
                    if (m_cursor_pos > 0)
                        m_cursor_pos--;
                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.keysym == XKB_KEY_Right || ev.keysym == XKB_KEY_KP_Right)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;
                    if (m_cursor_pos < (int)m_text.length())
                        m_cursor_pos++;
                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.keysym == XKB_KEY_Home || ev.keysym == XKB_KEY_KP_Home)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;
                    m_cursor_pos = 0;
                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.keysym == XKB_KEY_End || ev.keysym == XKB_KEY_KP_End)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;
                    m_cursor_pos = m_text.length();
                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (!ev.text.empty())
                {
                    // Filter out non-printable control characters if any (though xkb_utf8 usually
                    // only gives printables)
                    if (ev.text[0] >= 32 || ev.text[0] == '\t')
                    {
                        delete_selection();
                        m_text.insert(m_cursor_pos, ev.text);
                        m_cursor_pos += ev.text.length();
                        m_selection_anchor = -1;
                        invalidate();
                    }
                }
            });
    }

    void TextBox::set_placeholder(const std::string &placeholder)
    {
        m_placeholder = placeholder;
        invalidate();
    }

    const std::string &TextBox::placeholder() const
    {
        return m_placeholder;
    }

    void TextBox::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");

        Color bg_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color border_color = Color(0.6f, 0.6f, 0.6f, 1.0f); // Tiger dark border
        Color focus_color = Color(0.4f, 0.7f, 1.0f, 0.8f);

        int radius = 0; // Tiger has sharp 90-degree corners per user request

        // 1. Cursor Blinking Logic
        if (has_focus())
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_blink_time)
                    .count();
            if (elapsed >= 500)
            {
                m_cursor_visible = !m_cursor_visible;
                m_last_blink_time = now;
            }
        }
        else
        {
            m_cursor_visible = false;
        }

        // 2. Main Background
        gc.setColor(bg_color);
        gc.fillRect(m_x, m_y, m_width, m_height, {radius});

        // 3. Focus Ring
        if (has_focus())
        {
            gc.setColor(focus_color);
            // Draw inset by half the border width (3px) to stay within bounds and avoid ghosting
            gc.drawRect(m_x + 1, m_y + 1, m_width - 3, m_height - 3, {radius}, 4.0f);
        }
        else
        {
            // 4. Normal Border
            gc.setColor(border_color);
            gc.drawRect(m_x, m_y, m_width, m_height, {radius}, 1.0f);
        }

        // 5. Inset Shadow (More subtle for Tiger)
        gc.fillLinearGradientRect(m_x + 1, m_y + 1, m_width - 2, 4, Color(0.0f, 0.0f, 0.0f, 0.15f),
                                  Color(0.0f, 0.0f, 0.0f, 0.0f), true, {radius, radius, 0, 0});

        // 6. Text (with scrolling and placeholder)
        gc.save();
        gc.clip(m_x + 5, m_y + 5, m_width - 10, m_height - 10);

        int text_x_base = m_x + 8;
        int text_y = m_y + (m_height / 2.0f) + (font.size * 0.4f) - 2;

        gc.setDrawFont(font.family.c_str(), font.size * 0.8, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);

        TextMetrics total_metrics =
            gc.getTextMetrics(m_text.c_str(), font.family.c_str(), font.size * 0.8,
                              FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        std::string lead_to_cursor = m_text.substr(0, m_cursor_pos);
        TextMetrics cursor_metrics =
            gc.getTextMetrics(lead_to_cursor.c_str(), font.family.c_str(), font.size * 0.8,
                              FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        int visible_width = m_width - 16;
        int cursor_x_rel = cursor_metrics.width;

        // Adjust scroll offset to keep cursor visible
        if (cursor_x_rel < m_scroll_offset)
        {
            m_scroll_offset = cursor_x_rel;
        }
        else if (cursor_x_rel > m_scroll_offset + visible_width)
        {
            m_scroll_offset = cursor_x_rel - visible_width;
        }

        // Also ensure we don't have unnecessary whitespace at the end if the text is short
        if (total_metrics.width > visible_width)
        {
            if (m_scroll_offset > total_metrics.width - visible_width)
                m_scroll_offset = total_metrics.width - visible_width;
        }
        else
        {
            m_scroll_offset = 0;
        }

        int draw_text_x = text_x_base - m_scroll_offset;

        // 6. Draw Placeholder OR Text
        if (m_text.empty() && !m_placeholder.empty())
        {
            gc.setColor(0.6f, 0.6f, 0.6f, 1.0f); // Placeholder color
            gc.drawText(text_x_base, text_y, m_placeholder.c_str());
        }
        else
        {
            // 6.5. Selection highlight
            if (has_focus() && m_selection_anchor != -1 && m_selection_anchor != m_cursor_pos)
            {
                int sel_start_idx = std::min(m_selection_anchor, m_cursor_pos);
                int sel_end_idx = std::max(m_selection_anchor, m_cursor_pos);

                std::string lead_text = m_text.substr(0, sel_start_idx);
                std::string sel_text = m_text.substr(0, sel_end_idx);

                TextMetrics lead_m =
                    gc.getTextMetrics(lead_text.c_str(), font.family.c_str(), font.size * 0.8,
                                      FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                TextMetrics sel_m =
                    gc.getTextMetrics(sel_text.c_str(), font.family.c_str(), font.size * 0.8,
                                      FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

                int sel_x = draw_text_x + lead_m.width;
                int sel_w = sel_m.width - lead_m.width;

                gc.setColor(0.4f, 0.7f, 1.0f, 0.4f); // Tiger/Aqua selection blue
                gc.fillRect(sel_x, m_y + 6, sel_w, m_height - 12);
            }

            gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
            gc.drawText(draw_text_x, text_y, m_text.c_str());
        }

        // 7. Click & Selection handling
        if (m_has_pending_click)
        {
            int local_x = m_pending_click_x - draw_text_x;
            int best_index = 0;
            int min_dist = 999999;

            for (size_t i = 0; i <= m_text.length(); ++i)
            {
                std::string sub = m_text.substr(0, i);
                TextMetrics sub_metrics =
                    gc.getTextMetrics(sub.c_str(), font.family.c_str(), font.size * 0.8,
                                      FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                int dist = std::abs(sub_metrics.width - local_x);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    best_index = i;
                }
            }

            if (m_selection_anchor == -1)
            {
                m_selection_anchor = best_index;
            }

            m_cursor_pos = best_index;
            m_has_pending_click = false;
            m_cursor_visible = true;
            m_last_blink_time = std::chrono::steady_clock::now();
        }

        // 8. Cursor (if focused and visible)
        if (has_focus() && m_cursor_visible)
        {
            int cursor_x = draw_text_x + cursor_metrics.width;
            gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
            gc.fillRect(cursor_x, m_y + 8, 2, m_height - 16);
        }
        gc.restore();
    }

    void TextBox::set_text(const std::string &text)
    {
        m_text = text;
        m_cursor_pos = text.length();
        invalidate();
    }
    const std::string &TextBox::text() const
    {
        return m_text;
    }

} // namespace horizon
