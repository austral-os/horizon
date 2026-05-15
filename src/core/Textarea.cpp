#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Menu.hpp>
#include <horizon/Textarea.hpp>
#include <horizon/WaylandWindow.hpp>
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace horizon
{
    Textarea::Textarea() : Widget()
    {
        set_fixed_size(120); // Default height for multiline
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
            [this](MouseButtonEventContext &ev)
            {
                m_selection_anchor = -1;
                m_is_dragging = true;
                m_has_pending_click = true;
                m_pending_click_x = (int)ev.x;
                m_pending_click_y = (int)ev.y;
                invalidate();
            });

        when_mouse_drag.connect(
            [this](MouseMoveEventContext &ev)
            {
                if (m_is_dragging)
                {
                    m_has_pending_click = true;
                    m_pending_click_x = (int)ev.x;
                    m_pending_click_y = (int)ev.y;
                    invalidate();
                }
            });

        when_mouse_release.connect([this](MouseButtonEventContext &ev) { m_is_dragging = false; });

        when_right_click.connect(
            [this](MouseButtonEventContext &ev)
            {
                if (application())
                {
                    auto menu = std::make_unique<Menu>();
                    application()->show_context_menu(menu.release(), -1, -1, ev.serial, this);
                    ev.stop_propagation = true;
                }
            });

        when_key_press.connect(
            [this](KeyEventContext &ev)
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
                    KeyEventContext ctx = ev;
                    when_text_changed.run(ctx);
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
                    KeyEventContext ctx = ev;
                    when_text_changed.run(ctx);
                }
                else if (ev.keysym == XKB_KEY_Return || ev.keysym == XKB_KEY_KP_Enter)
                {
                    delete_selection();
                    m_text.insert(m_cursor_pos, "\n");
                    m_cursor_pos++;
                    invalidate();
                    KeyEventContext ctx = ev;
                    when_text_changed.run(ctx);
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

                    // Home moves to start of line or start of text?
                    // Let's implement it to start of line for now.
                    size_t prev_newline =
                        m_text.find_last_of('\n', (m_cursor_pos > 0) ? m_cursor_pos - 1 : 0);
                    if (prev_newline == std::string::npos)
                        m_cursor_pos = 0;
                    else
                        m_cursor_pos = prev_newline + 1;

                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.keysym == XKB_KEY_End || ev.keysym == XKB_KEY_KP_End)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;

                    size_t next_newline = m_text.find_first_of('\n', m_cursor_pos);
                    if (next_newline == std::string::npos)
                        m_cursor_pos = m_text.length();
                    else
                        m_cursor_pos = next_newline;

                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.keysym == XKB_KEY_Up || ev.keysym == XKB_KEY_KP_Up)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;

                    int current_line_idx = -1;
                    for (int i = 0; i < (int)m_cached_lines.size(); ++i)
                    {
                        if (m_cursor_pos >= m_cached_lines[i].start_index &&
                            m_cursor_pos <=
                                m_cached_lines[i].start_index + m_cached_lines[i].length)
                        {
                            current_line_idx = i;
                            break;
                        }
                    }

                    if (current_line_idx > 0)
                    {
                        const auto &curr_line = m_cached_lines[current_line_idx];
                        const auto &prev_line = m_cached_lines[current_line_idx - 1];
                        int x_offset = m_cursor_pos - curr_line.start_index;
                        m_cursor_pos = prev_line.start_index + std::min(x_offset, prev_line.length);
                    }

                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.keysym == XKB_KEY_Down || ev.keysym == XKB_KEY_KP_Down)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;

                    int current_line_idx = -1;
                    for (int i = 0; i < (int)m_cached_lines.size(); ++i)
                    {
                        if (m_cursor_pos >= m_cached_lines[i].start_index &&
                            m_cursor_pos <=
                                m_cached_lines[i].start_index + m_cached_lines[i].length)
                        {
                            current_line_idx = i;
                            break;
                        }
                    }

                    if (current_line_idx != -1 && current_line_idx < (int)m_cached_lines.size() - 1)
                    {
                        const auto &curr_line = m_cached_lines[current_line_idx];
                        const auto &next_line = m_cached_lines[current_line_idx + 1];
                        int x_offset = m_cursor_pos - curr_line.start_index;
                        m_cursor_pos = next_line.start_index + std::min(x_offset, next_line.length);
                    }

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
                        if (ev.text[0] == '\t')
                            ev.stop_propagation = true;

                        invalidate();
                        KeyEventContext ctx = ev;
                        when_text_changed.run(ctx);
                    }
                }
            });
    }

    std::vector<Textarea::LineInfo> Textarea::layout_text(GraphicsContext &gc, int width_limit)
    {
        std::vector<LineInfo> lines;
        int current_y = 0;

        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");
        int font_size = font.size * 0.8;
        int line_height = font_size + m_line_spacing;

        if (m_text.empty())
        {
            lines.push_back({"", 0, 0, 0});
            return lines;
        }

        size_t last_pos = 0;
        size_t next_pos = 0;
        while ((next_pos = m_text.find('\n', last_pos)) != std::string::npos)
        {
            std::string hard_line = m_text.substr(last_pos, next_pos - last_pos);

            // Apply soft-wrap to this hard line
            if (width_limit > 20 && !hard_line.empty())
            {
                std::string current_soft_line;
                int soft_line_start_idx = last_pos;

                size_t word_start_in_hard = 0;
                while (word_start_in_hard < hard_line.length())
                {
                    size_t word_end = hard_line.find_first_of(" \t", word_start_in_hard);
                    if (word_end == std::string::npos)
                        word_end = hard_line.length();

                    std::string word =
                        hard_line.substr(word_start_in_hard, word_end - word_start_in_hard);
                    std::string space =
                        (word_end < hard_line.length()) ? hard_line.substr(word_end, 1) : "";

                    std::string test_line = current_soft_line + word;
                    auto metrics =
                        gc.getTextMetrics(test_line.c_str(), font.family.c_str(), font_size,
                                          FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

                    if (metrics.width > width_limit && !current_soft_line.empty())
                    {
                        lines.push_back({current_soft_line, soft_line_start_idx,
                                         (int)current_soft_line.length(), current_y});
                        current_y += line_height;
                        soft_line_start_idx += current_soft_line.length();
                        current_soft_line = word + space;
                    }
                    else
                    {
                        current_soft_line += word + space;
                    }

                    word_start_in_hard = word_end + space.length();
                }
                if (!current_soft_line.empty())
                {
                    lines.push_back({current_soft_line, soft_line_start_idx,
                                     (int)current_soft_line.length(), current_y});
                    current_y += line_height;
                }
            }
            else
            {
                lines.push_back({hard_line, (int)last_pos, (int)hard_line.length(), current_y});
                current_y += line_height;
            }

            last_pos = next_pos + 1;
        }

        // Last segment (trailing line)
        std::string hard_line = m_text.substr(last_pos);
        if (width_limit > 20 && !hard_line.empty())
        {
            std::string current_soft_line;
            int soft_line_start_idx = last_pos;
            size_t word_start_in_hard = 0;
            while (word_start_in_hard < hard_line.length())
            {
                size_t word_end = hard_line.find_first_of(" \t", word_start_in_hard);
                if (word_end == std::string::npos)
                    word_end = hard_line.length();

                std::string word =
                    hard_line.substr(word_start_in_hard, word_end - word_start_in_hard);
                std::string space =
                    (word_end < hard_line.length()) ? hard_line.substr(word_end, 1) : "";

                std::string test_line = current_soft_line + word;
                auto metrics = gc.getTextMetrics(test_line.c_str(), font.family.c_str(), font_size,
                                                 FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

                if (metrics.width > width_limit && !current_soft_line.empty())
                {
                    lines.push_back({current_soft_line, soft_line_start_idx,
                                     (int)current_soft_line.length(), current_y});
                    current_y += line_height;
                    soft_line_start_idx += current_soft_line.length();
                    current_soft_line = word + space;
                }
                else
                {
                    current_soft_line += word + space;
                }
                word_start_in_hard = word_end + space.length();
            }
            lines.push_back({current_soft_line, soft_line_start_idx,
                             (int)current_soft_line.length(), current_y});
        }
        else
        {
            lines.push_back({hard_line, (int)last_pos, (int)hard_line.length(), current_y});
        }

        return lines;
    }

    void Textarea::draw(GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");

        Color bg_color = tm->get_color("textbox_bg");
        Color border_color = tm->get_color("textbox_brd");
        Color focus_color = tm->get_color("textbox_focus");

        gc.setColor(bg_color);
        gc.fillRect(m_x, m_y, m_width, m_height, {m_corner_radius});

        if (has_focus())
        {
            gc.setColor(focus_color);
            gc.drawRect(m_x + 1, m_y + 1, m_width - 3, m_height - 3, {m_corner_radius}, 4.0f);

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
            gc.setColor(border_color);
            gc.drawRect(m_x, m_y, m_width, m_height, {m_corner_radius}, 1.0f);
            m_cursor_visible = false;
        }

        gc.save();
        gc.clip(m_x + 2, m_y + 2, m_width - 4, m_height - 4);

        int font_size = font.size * 0.8;
        gc.setDrawFont(font.family.c_str(), font_size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        int width_limit = m_width - m_padding_left - m_padding_right;
        if (m_last_width_limit != width_limit || m_last_text_layout != m_text)
        {
            m_cached_lines = layout_text(gc, width_limit);
            m_last_width_limit = width_limit;
            m_last_text_layout = m_text;
        }
        const auto &lines = m_cached_lines;

        if (m_has_pending_click)
        {
            update_cursor_from_pending_click(lines, gc);
        }

        ensure_cursor_visible(lines, m_height - m_padding_top - m_padding_bottom);

        int draw_y_base = m_y + m_padding_top - m_scroll_offset_y;

        if (m_text.empty() && !m_placeholder.empty())
        {
            gc.setColor(0.6f, 0.6f, 0.6f, 1.0f);
            gc.drawText(m_x + m_padding_left, draw_y_base + font_size, m_placeholder.c_str());
        }
        else
        {
            for (const auto &line : lines)
            {
                int line_y = draw_y_base + line.y_offset;

                // Only draw if visible
                if (line_y + font_size + m_line_spacing > m_y && line_y < m_y + m_height)
                {
                    // Selection handling per line
                    if (has_focus() && m_selection_anchor != -1 &&
                        m_selection_anchor != m_cursor_pos)
                    {
                        int sel_min = std::min(m_selection_anchor, m_cursor_pos);
                        int sel_max = std::max(m_selection_anchor, m_cursor_pos);

                        int line_start = line.start_index;
                        int line_end = line.start_index + line.length;

                        // Check if selection overlaps with this line
                        if (sel_min < line_end && sel_max > line_start)
                        {
                            int start_in_line = std::max(0, sel_min - line_start);
                            int end_in_line = std::min(line.length, sel_max - line_start);

                            std::string lead = line.text.substr(0, start_in_line);
                            std::string sel = line.text.substr(0, end_in_line);

                            auto lead_m =
                                gc.getTextMetrics(lead.c_str(), font.family.c_str(), font_size,
                                                  FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                            auto sel_m =
                                gc.getTextMetrics(sel.c_str(), font.family.c_str(), font_size,
                                                  FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

                            gc.setColor(0.4f, 0.7f, 1.0f, 0.4f);

                            // If selection goes to next line, extend the highlight box
                            int extra_w = (sel_max > line_end && line_end < (int)m_text.length() &&
                                           m_text[line_end] != '\n')
                                              ? 0
                                              : (sel_max > line_end ? 8 : 0);

                            gc.fillRect(m_x + m_padding_left + lead_m.width, line_y,
                                        sel_m.width - lead_m.width + extra_w,
                                        font_size + m_line_spacing);
                        }
                    }

                    gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
                    gc.drawText(m_x + m_padding_left, line_y + font_size - 2, line.text.c_str());
                }
            }
        }

        // Draw cursor
        if (has_focus() && m_cursor_visible)
        {
            bool cursor_drawn = false;
            for (const auto &line : lines)
            {
                if (m_cursor_pos >= line.start_index &&
                    m_cursor_pos <= line.start_index + line.length)
                {
                    // Special case: if cursor is at the very end of a line that was wrapped,
                    // we prefer to draw it at the start of the next line IF it's not a hard newline
                    if (m_cursor_pos == line.start_index + line.length &&
                        m_cursor_pos < (int)m_text.length() && m_text[m_cursor_pos] != '\n' &&
                        &line != &lines.back())
                    {
                        continue;
                    }

                    std::string lead = line.text.substr(0, m_cursor_pos - line.start_index);
                    auto m = gc.getTextMetrics(lead.c_str(), font.family.c_str(), font_size,
                                               FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

                    int cursor_x = m_x + m_padding_left + m.width;
                    int cursor_y = draw_y_base + line.y_offset;

                    gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
                    gc.fillRect(cursor_x, cursor_y + 2, 2, font_size);
                    cursor_drawn = true;
                    break;
                }
            }
            // Fallback for end of text if not drawn
            if (!cursor_drawn && !lines.empty())
            {
                const auto &line = lines.back();
                std::string lead = line.text.substr(0, m_cursor_pos - line.start_index);
                auto m = gc.getTextMetrics(lead.c_str(), font.family.c_str(), font_size,
                                           FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                int cursor_x = m_x + m_padding_left + m.width;
                int cursor_y = draw_y_base + line.y_offset;
                gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
                gc.fillRect(cursor_x, cursor_y + 2, 2, font_size);
            }
        }

        gc.restore();
    }

    void Textarea::update_cursor_from_pending_click(const std::vector<LineInfo> &lines,
                                                    GraphicsContext &gc)
    {
        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");
        int font_size = font.size * 0.8;
        int line_height = font_size + m_line_spacing;

        int local_y = m_pending_click_y - (m_y + m_padding_top - m_scroll_offset_y);

        int line_idx = std::clamp(local_y / line_height, 0, (int)lines.size() - 1);
        const auto &line = lines[line_idx];

        int local_x = m_pending_click_x - (m_x + m_padding_left);
        int best_index = 0;
        int min_dist = 999999;

        for (size_t i = 0; i <= line.text.length(); ++i)
        {
            std::string sub = line.text.substr(0, i);
            TextMetrics sub_metrics = gc.getTextMetrics(sub.c_str(), font.family.c_str(), font_size,
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
            m_selection_anchor = line.start_index + best_index;
        }

        m_cursor_pos = line.start_index + best_index;
        m_has_pending_click = false;
        m_cursor_visible = true;
        m_last_blink_time = std::chrono::steady_clock::now();
    }

    void Textarea::ensure_cursor_visible(const std::vector<LineInfo> &lines, int visible_height)
    {
        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("window");
        int font_size = font.size * 0.8;
        int line_height = font_size + m_line_spacing;

        for (const auto &line : lines)
        {
            if (m_cursor_pos >= line.start_index && m_cursor_pos <= line.start_index + line.length)
            {
                int cursor_y_top = line.y_offset;
                int cursor_y_bottom = line.y_offset + line_height;

                if (cursor_y_top < m_scroll_offset_y)
                {
                    m_scroll_offset_y = cursor_y_top;
                }
                else if (cursor_y_bottom > m_scroll_offset_y + visible_height)
                {
                    m_scroll_offset_y = cursor_y_bottom - visible_height;
                }
                break;
            }
        }
    }

    void Textarea::set_text(const std::string &text)
    {
        m_text = text;
        m_cursor_pos = text.length();
        invalidate();
    }

    const std::string &Textarea::text() const
    {
        return m_text;
    }

    void Textarea::set_placeholder(const std::string &placeholder)
    {
        m_placeholder = placeholder;
        invalidate();
    }

    const std::string &Textarea::placeholder() const
    {
        return m_placeholder;
    }

    void Textarea::move_cursor_to_start()
    {
        m_cursor_pos = 0;
        m_selection_anchor = -1;
        m_scroll_offset_y = 0;
        invalidate();
    }

    void Textarea::move_cursor_to_end()
    {
        m_cursor_pos = (int)m_text.length();
        m_selection_anchor = -1;
        invalidate();
    }

    bool Textarea::can_perform(ClipboardAction action) const
    {
        if (action == ClipboardAction::Copy || action == ClipboardAction::Cut)
        {
            return m_selection_anchor != -1 && m_selection_anchor != m_cursor_pos;
        }
        if (action == ClipboardAction::Paste)
        {
            if (application())
            {
                auto mimes = application()->get_clipboard_mime_types();
                for (const auto &m : mimes)
                {
                    if (m == "text/plain")
                        return true;
                }
                return false;
            }
            return true;
        }
        return false;
    }

    void Textarea::perform(ClipboardAction action)
    {
        if (action == ClipboardAction::Copy || action == ClipboardAction::Cut)
        {
            if (m_selection_anchor != -1 && m_selection_anchor != m_cursor_pos)
            {
                if (application())
                {
                    application()->set_clipboard_owner(this);
                    if (action == ClipboardAction::Cut)
                    {
                        int start = std::min(m_selection_anchor, m_cursor_pos);
                        int end = std::max(m_selection_anchor, m_cursor_pos);
                        m_text.erase(start, end - start);
                        m_cursor_pos = start;
                        m_selection_anchor = -1;
                        invalidate();
                        KeyEventContext ev;
                        when_text_changed.run(ev);
                    }
                }
            }
        }
        else if (action == ClipboardAction::Paste)
        {
            if (application())
            {
                application()->request_clipboard_data(this, "text/plain");
            }
        }
    }

    void Textarea::provide_clipboard_data(const std::string &mime, DataSink &sink)
    {
        if (mime == "text/plain")
        {
            if (m_selection_anchor != -1 && m_selection_anchor != m_cursor_pos)
            {
                int start = std::min(m_selection_anchor, m_cursor_pos);
                int end = std::max(m_selection_anchor, m_cursor_pos);
                std::string sel = m_text.substr(start, end - start);
                std::vector<uint8_t> data(sel.begin(), sel.end());
                sink.write(data);
                sink.done();
            }
        }
    }

    std::vector<std::string> Textarea::provided_mime_types() const
    {
        return {"text/plain"};
    }

    std::vector<std::string> Textarea::accepted_mime_types() const
    {
        return {"text/plain"};
    }

    void Textarea::on_clipboard_data_received(const std::string &mime,
                                              const std::vector<uint8_t> &data)
    {
        if (mime == "text/plain" && !data.empty())
        {
            std::string pasted(data.begin(), data.end());

            // Delete selection if any
            if (m_selection_anchor != -1 && m_selection_anchor != m_cursor_pos)
            {
                int start = std::min(m_selection_anchor, m_cursor_pos);
                int end = std::max(m_selection_anchor, m_cursor_pos);
                m_text.erase(start, end - start);
                m_cursor_pos = start;
                m_selection_anchor = -1;
            }

            m_text.insert(m_cursor_pos, pasted);
            m_cursor_pos += pasted.length();
            invalidate();
            KeyEventContext ev;
            when_text_changed.run(ev);
        }
    }

} // namespace horizon
