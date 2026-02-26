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
        {KEY_1, '1'}, {KEY_2, '2'},     {KEY_3, '3'},   {KEY_4, '4'},     {KEY_5, '5'},
        {KEY_6, '6'}, {KEY_7, '7'},     {KEY_8, '8'},   {KEY_9, '9'},     {KEY_0, '0'},
        {KEY_Q, 'q'}, {KEY_W, 'w'},     {KEY_E, 'e'},   {KEY_R, 'r'},     {KEY_T, 't'},
        {KEY_Y, 'y'}, {KEY_U, 'u'},     {KEY_I, 'i'},   {KEY_O, 'o'},     {KEY_P, 'p'},
        {KEY_A, 'a'}, {KEY_S, 's'},     {KEY_D, 'd'},   {KEY_F, 'f'},     {KEY_G, 'g'},
        {KEY_H, 'h'}, {KEY_J, 'j'},     {KEY_K, 'k'},   {KEY_L, 'l'},     {KEY_Z, 'z'},
        {KEY_X, 'x'}, {KEY_C, 'c'},     {KEY_V, 'v'},   {KEY_B, 'b'},     {KEY_N, 'n'},
        {KEY_M, 'm'}, {KEY_SPACE, ' '}, {KEY_DOT, '.'}, {KEY_COMMA, ','}, {KEY_MINUS, '-'}};

    TextBox::TextBox() : Widget()
    {
        set_fixed_size(40);
        set_cursor_type(CursorType::Text);

        when_key_press.connect(
            [this](EventContext &ev)
            {
                if (!has_focus())
                    return;

                if (ev.key == KEY_BACKSPACE)
                {
                    if (!m_text.empty())
                    {
                        m_text.pop_back();
                        invalidate();
                    }
                }
                else
                {
                    auto it = KEY_MAP.find(ev.key);
                    if (it != KEY_MAP.end())
                    {
                        m_text += it->second;
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

        // 1. Main Background
        gc.setColor(bg_color);
        gc.fillRect(m_x, m_y, m_width, m_height, {radius});

        // 2. Focus Ring
        if (has_focus())
        {
            gc.setColor(focus_color);
            // Draw inset by half the border width (3px) to stay within bounds and avoid ghosting
            gc.drawRect(m_x + 3, m_y + 3, m_width - 6, m_height - 6, {radius}, 6.0f);
        }
        else
        {
            // 3. Normal Border
            gc.setColor(border_color);
            gc.drawRect(m_x, m_y, m_width, m_height, {radius}, 1.0f);
        }

        // 4. Inset Shadow (More subtle for Tiger)
        gc.fillLinearGradientRect(m_x + 1, m_y + 1, m_width - 2, 4, Color(0.0f, 0.0f, 0.0f, 0.15f),
                                  Color(0.0f, 0.0f, 0.0f, 0.0f), true, {radius, radius, 0, 0});

        // 5. Text (with scrolling and placeholder)
        gc.save();
        gc.clip(m_x + 5, m_y + 5, m_width - 10, m_height - 10);

        int text_x_base = m_x + 8;
        int text_y = m_y + (m_height / 2) + (font.size * 0.4) - 2;

        if (m_text.empty() && !m_placeholder.empty())
        {
            gc.setColor(0.6f, 0.6f, 0.6f, 1.0f); // Placeholder color
            gc.setDrawFont(font.family.c_str(), font.size * 0.8, FONT_SLANT_NORMAL,
                           FONT_WEIGHT_NORMAL);
            gc.drawText(text_x_base, text_y, m_placeholder.c_str());
        }
        else
        {
            gc.setColor(0.0f, 0.0f, 0.0f, 1.0f);
            gc.setDrawFont(font.family.c_str(), font.size * 0.8, FONT_SLANT_NORMAL,
                           FONT_WEIGHT_NORMAL);

            TextMetrics metrics =
                gc.getTextMetrics(m_text.c_str(), font.family.c_str(), font.size * 0.8,
                                  FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

            int available_width = m_width - 16;
            int scroll_offset = 0;
            if (metrics.width > available_width)
            {
                scroll_offset = metrics.width - available_width;
            }

            int draw_text_x = text_x_base - scroll_offset;
            gc.drawText(draw_text_x, text_y, m_text.c_str());

            // 6. Cursor (if focused)
            if (has_focus())
            {
                int cursor_x = draw_text_x + metrics.width + 1;
                gc.setColor(0.0f, 0.0f, 0.0f, 1.0f); // Match text color per user request
                gc.fillRect(cursor_x, m_y + 8, 2, m_height - 16);
            }
        }
        gc.restore();
    }

    void TextBox::set_text(const std::string &text)
    {
        m_text = text;
        invalidate();
    }
    const std::string &TextBox::text() const
    {
        return m_text;
    }

} // namespace horizon
