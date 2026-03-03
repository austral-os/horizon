#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/TextBox.hpp>
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace horizon
{
    TextBoxBase::TextBoxBase() : Widget()
    {
        set_fixed_size(40);
        set_cursor_type(CursorType::Text);
        set_focusable(true);

        m_last_blink_time = std::chrono::steady_clock::now();

        when_focus.connect(
            [this](EventContext &)
            {
                m_cursor_visible = true;
                m_last_blink_time = std::chrono::steady_clock::now();
                invalidate();
            });

        when_blur.connect(
            [this](EventContext &)
            {
                m_cursor_visible = false;
                invalidate();
            });

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
                    when_text_changed.run(ev);
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
                    when_text_changed.run(ev);
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
                    if (ev.text[0] >= 32 || ev.text[0] == '\t')
                    {
                        delete_selection();
                        m_text.insert(m_cursor_pos, ev.text);
                        m_cursor_pos += ev.text.length();
                        m_selection_anchor = -1;
                        invalidate();
                        EventContext ctx = ev;
                        when_text_changed.run(ctx);
                    }
                }
            });
    }

    void TextBoxBase::set_placeholder(const std::string &placeholder)
    {
        m_placeholder = placeholder;
        invalidate();
    }

    const std::string &TextBoxBase::placeholder() const
    {
        return m_placeholder;
    }

    void TextBoxBase::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");

        Color bg_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color border_color = Color(0.6f, 0.6f, 0.6f, 1.0f);
        Color focus_color = Color(0.4f, 0.7f, 1.0f, 0.8f);   // Blue when valid and focused
        Color invalid_color = Color(1.0f, 0.3f, 0.3f, 1.0f); // Red when invalid

        bool valid = is_valid();

        int radius = 0;

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

        gc.setColor(bg_color);
        gc.fillRect(m_x, m_y, m_width, m_height, {m_corner_radius});

        if (!valid)
        {
            gc.setColor(invalid_color);
            gc.drawRect(m_x, m_y, m_width, m_height, {m_corner_radius}, 2.0f);
        }
        else if (has_focus())
        {
            gc.setColor(focus_color);
            gc.drawRect(m_x + 1, m_y + 1, m_width - 3, m_height - 3, {m_corner_radius}, 4.0f);
        }
        else
        {
            gc.setColor(border_color);
            gc.drawRect(m_x, m_y, m_width, m_height, {m_corner_radius}, 1.0f);
        }

        gc.fillLinearGradientRect(m_x + 1, m_y + 1, m_width - 2, 4, Color(0.0f, 0.0f, 0.0f, 0.15f),
                                  Color(0.0f, 0.0f, 0.0f, 0.0f), true,
                                  {m_corner_radius, m_corner_radius, 0, 0});

        gc.save();
        gc.clip(m_x + m_padding_left - 3, m_y + 5, m_width - (m_padding_left + m_padding_right) + 6,
                m_height - 10);

        int text_x_base = m_x + m_padding_left;
        int text_y = m_y + (m_height / 2.0f) + (font.size * 0.4f) - 2;

        gc.setDrawFont(font.family.c_str(), font.size * 0.8, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        std::string display_text = get_display_text();

        TextMetrics total_metrics =
            gc.getTextMetrics(display_text.c_str(), font.family.c_str(), font.size * 0.8,
                              FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        std::string lead_to_cursor = display_text.substr(0, m_cursor_pos);
        TextMetrics cursor_metrics =
            gc.getTextMetrics(lead_to_cursor.c_str(), font.family.c_str(), font.size * 0.8,
                              FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        int visible_width = m_width - (m_padding_left + m_padding_right);
        int cursor_x_rel = cursor_metrics.width;

        if (cursor_x_rel < m_scroll_offset)
        {
            m_scroll_offset = cursor_x_rel;
        }
        else if (cursor_x_rel > m_scroll_offset + visible_width)
        {
            m_scroll_offset = cursor_x_rel - visible_width;
        }

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

        if (m_text.empty() && !m_placeholder.empty())
        {
            gc.setColor(0.6f, 0.6f, 0.6f, 1.0f);
            gc.drawText(text_x_base, text_y, m_placeholder.c_str());
        }
        else
        {
            if (has_focus() && m_selection_anchor != -1 && m_selection_anchor != m_cursor_pos)
            {
                int sel_start_idx = std::min(m_selection_anchor, m_cursor_pos);
                int sel_end_idx = std::max(m_selection_anchor, m_cursor_pos);

                std::string lead_text = display_text.substr(0, sel_start_idx);
                std::string sel_text = display_text.substr(0, sel_end_idx);

                TextMetrics lead_m =
                    gc.getTextMetrics(lead_text.c_str(), font.family.c_str(), font.size * 0.8,
                                      FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                TextMetrics sel_m =
                    gc.getTextMetrics(sel_text.c_str(), font.family.c_str(), font.size * 0.8,
                                      FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

                int sel_x = draw_text_x + lead_m.width;
                int sel_w = sel_m.width - lead_m.width;

                gc.setColor(0.4f, 0.7f, 1.0f, 0.4f);
                gc.fillRect(sel_x, m_y + 6, sel_w, m_height - 12);
            }

            gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
            gc.drawText(draw_text_x, text_y, display_text.c_str());
        }

        if (m_has_pending_click)
        {
            int local_x = m_pending_click_x - draw_text_x;
            int best_index = 0;
            int min_dist = 999999;

            for (size_t i = 0; i <= display_text.length(); ++i)
            {
                std::string sub = display_text.substr(0, i);
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

        if (has_focus() && m_cursor_visible)
        {
            int cursor_x = draw_text_x + cursor_metrics.width;
            gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
            gc.fillRect(cursor_x, m_y + 8, 2, m_height - 16);
        }
        gc.restore();
    }

    void TextBoxBase::set_text(const std::string &text)
    {
        m_text = text;
        m_cursor_pos = text.length();
        invalidate();
    }
    const std::string &TextBoxBase::text() const
    {
        return m_text;
    }

} // namespace horizon
