#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/TextBox.hpp>
#include <linux/input-event-codes.h>
#include <unordered_map>

namespace horizon
{
    // A simple mapping from Linux keycodes to ASCII characters.
    // In a real application, this would be handled by a library like xkbcommon.
    static const std::unordered_map<uint32_t, char> KEY_MAP = {
        {KEY_1, '1'},          {KEY_2, '2'},          {KEY_3, '3'},         {KEY_4, '4'},
        {KEY_5, '5'},          {KEY_6, '6'},          {KEY_7, '7'},         {KEY_8, '8'},
        {KEY_9, '9'},          {KEY_0, '0'},          {KEY_Q, 'q'},         {KEY_W, 'w'},
        {KEY_E, 'e'},          {KEY_R, 'r'},          {KEY_T, 't'},         {KEY_Y, 'y'},
        {KEY_U, 'u'},          {KEY_I, 'i'},          {KEY_O, 'o'},         {KEY_P, 'p'},
        {KEY_A, 'a'},          {KEY_S, 's'},          {KEY_D, 'd'},         {KEY_F, 'f'},
        {KEY_G, 'g'},          {KEY_H, 'h'},          {KEY_J, 'j'},         {KEY_K, 'k'},
        {KEY_L, 'l'},          {KEY_Z, 'z'},          {KEY_X, 'x'},         {KEY_C, 'c'},
        {KEY_V, 'v'},          {KEY_B, 'b'},          {KEY_N, 'n'},         {KEY_M, 'm'},
        {KEY_SPACE, ' '},      {KEY_DOT, '.'},        {KEY_COMMA, ','},     {KEY_MINUS, '-'},
        {KEY_SLASH, '-'},      {KEY_EQUAL, '='},      {KEY_SEMICOLON, ';'}, {KEY_APOSTROPHE, '\''},
        {KEY_GRAVE, '`'},      {KEY_BACKSLASH, '\\'}, {KEY_LEFTBRACE, '['}, {KEY_RIGHTBRACE, ']'},
        {KEY_KP0, '0'},        {KEY_KP1, '1'},        {KEY_KP2, '2'},       {KEY_KP3, '3'},
        {KEY_KP4, '4'},        {KEY_KP5, '5'},        {KEY_KP6, '6'},       {KEY_KP7, '7'},
        {KEY_KP8, '8'},        {KEY_KP9, '9'},        {KEY_KPDOT, '.'},     {KEY_KPSLASH, '/'},
        {KEY_KPASTERISK, '*'}, {KEY_KPMINUS, '-'},    {KEY_KPPLUS, '+'}};

    static const std::unordered_map<uint32_t, char> SHIFT_KEY_MAP = {
        {KEY_1, '!'},          {KEY_2, '@'},         {KEY_3, '#'},         {KEY_4, '$'},
        {KEY_5, '%'},          {KEY_6, '^'},         {KEY_7, '&'},         {KEY_8, '*'},
        {KEY_9, '('},          {KEY_0, ')'},         {KEY_Q, 'Q'},         {KEY_W, 'W'},
        {KEY_E, 'E'},          {KEY_R, 'R'},         {KEY_T, 'T'},         {KEY_Y, 'Y'},
        {KEY_U, 'U'},          {KEY_I, 'I'},         {KEY_O, 'O'},         {KEY_P, 'P'},
        {KEY_A, 'A'},          {KEY_S, 'S'},         {KEY_D, 'D'},         {KEY_F, 'F'},
        {KEY_G, 'G'},          {KEY_H, 'H'},         {KEY_J, 'J'},         {KEY_K, 'K'},
        {KEY_L, 'L'},          {KEY_Z, 'Z'},         {KEY_X, 'X'},         {KEY_C, 'C'},
        {KEY_V, 'V'},          {KEY_B, 'B'},         {KEY_N, 'N'},         {KEY_M, 'M'},
        {KEY_SPACE, ' '},      {KEY_DOT, '>'},       {KEY_COMMA, '<'},     {KEY_MINUS, '_'},
        {KEY_SLASH, '_'},      {KEY_EQUAL, '+'},     {KEY_SEMICOLON, ':'}, {KEY_APOSTROPHE, '"'},
        {KEY_GRAVE, '~'},      {KEY_BACKSLASH, '|'}, {KEY_LEFTBRACE, '{'}, {KEY_RIGHTBRACE, '}'},
        {KEY_KP0, '0'},        {KEY_KP1, '1'},       {KEY_KP2, '2'},       {KEY_KP3, '3'},
        {KEY_KP4, '4'},        {KEY_KP5, '5'},       {KEY_KP6, '6'},       {KEY_KP7, '7'},
        {KEY_KP8, '8'},        {KEY_KP9, '9'},       {KEY_KPDOT, '.'},     {KEY_KPSLASH, '/'},
        {KEY_KPASTERISK, '*'}, {KEY_KPMINUS, '-'},   {KEY_KPPLUS, '+'}};

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

                if (ev.key == KEY_BACKSPACE)
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
                else if (ev.key == KEY_DELETE)
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
                else if (ev.key == KEY_LEFT)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;
                    if (m_cursor_pos > 0)
                        m_cursor_pos--;
                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.key == KEY_RIGHT)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;
                    if (m_cursor_pos < (int)m_text.length())
                        m_cursor_pos++;
                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.key == KEY_HOME)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;
                    m_cursor_pos = 0;
                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.key == KEY_END)
                {
                    if (shift && m_selection_anchor == -1)
                        m_selection_anchor = m_cursor_pos;
                    m_cursor_pos = m_text.length();
                    if (!shift)
                        m_selection_anchor = -1;
                    invalidate();
                }
                else if (ev.key != KEY_LEFTSHIFT && ev.key != KEY_RIGHTSHIFT &&
                         ev.key != KEY_LEFTCTRL && ev.key != KEY_RIGHTCTRL &&
                         ev.key != KEY_LEFTALT && ev.key != KEY_RIGHTALT && ev.key != KEY_CAPSLOCK)
                {
                    bool caps = ev.modifiers & 0x8; // Application::CAPSLOCK is bit 3
                    bool is_alpha = (ev.key >= KEY_Q && ev.key <= KEY_P) ||
                                    (ev.key >= KEY_A && ev.key <= KEY_L) ||
                                    (ev.key >= KEY_Z && ev.key <= KEY_M);

                    bool use_shift_map = is_alpha ? (shift ^ caps) : shift;

                    const auto &map = use_shift_map ? SHIFT_KEY_MAP : KEY_MAP;
                    auto it = map.find(ev.key);
                    if (it != map.end())
                    {
                        delete_selection();
                        m_text.insert(m_cursor_pos, 1, it->second);
                        m_cursor_pos++;
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
        int text_y = m_y + (m_height / 2) + (font.size * 0.4) - 2;

        gc.setDrawFont(font.family.c_str(), font.size * 0.8, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

        if (m_text.empty() && !m_placeholder.empty())
        {
            gc.setColor(0.6f, 0.6f, 0.6f, 1.0f); // Placeholder color
            gc.drawText(text_x_base, text_y, m_placeholder.c_str());
        }
        else
        {
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

            // 6.5. Selection highlight
            if (has_focus() && m_selection_anchor != -1 && m_selection_anchor != m_cursor_pos)
            {
                int sel_start_idx = std::min(m_selection_anchor, m_cursor_pos);
                int sel_end_idx = std::max(m_selection_anchor, m_cursor_pos);

                std::string lead_text = m_text.substr(0, sel_start_idx);
                std::string sel_text = m_text.substr(0, sel_end_idx);

                TextMetrics lead_metrics =
                    gc.getTextMetrics(lead_text.c_str(), font.family.c_str(), font.size * 0.8,
                                      FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                TextMetrics sel_metrics =
                    gc.getTextMetrics(sel_text.c_str(), font.family.c_str(), font.size * 0.8,
                                      FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

                int sel_x = draw_text_x + lead_metrics.width;
                int sel_w = sel_metrics.width - lead_metrics.width;

                gc.setColor(0.4f, 0.7f, 1.0f, 0.4f); // Tiger/Aqua selection blue
                gc.fillRect(sel_x, m_y + 6, sel_w, m_height - 12);
            }

            gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
            gc.drawText(draw_text_x, text_y, m_text.c_str());

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
                std::string lead_text = m_text.substr(0, m_cursor_pos);
                TextMetrics cursor_metrics =
                    gc.getTextMetrics(lead_text.c_str(), font.family.c_str(), font.size * 0.8,
                                      FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

                int cursor_x = draw_text_x + cursor_metrics.width;
                gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
                gc.fillRect(cursor_x, m_y + 8, 2, m_height - 16);
            }
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
