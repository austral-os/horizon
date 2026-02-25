#pragma once
#include "horizon/AquaObject.hpp"
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{

    template <typename T> class Button : public T
    {
    public:
        Button() : T()
        {
            this->when_mouse_enter.connect([this](EventContext &) { this->invalidate(); });
            this->when_mouse_leave.connect([this](EventContext &) { this->invalidate(); });

            if (std::is_same<T, AquaObject>::value)
            {
                this->set_corner_radius({10, 10, 10, 10});
            }
            else if (std::is_same<T, SolidObject>::value)
            {
                this->set_corner_radius({6, 6, 6, 6});
            }
        }
        ~Button() = default;

        void draw(GraphicsContext &gc) override
        {
            auto *tm = this->application()->theme_manager.get();
            auto font = tm->get_font("window");

            Color window_fg = tm->get_color("window_fg");
            Color text_color = window_fg;

            T::draw(gc);

            // Center the text
            TextMetrics metrics = gc.getTextMetrics(m_text.c_str(), font.family.c_str(), font.size,
                                                    FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);

            int text_x = this->m_start_draw_x + (this->m_width / 2) - (metrics.width / 2);
            int text_y = this->m_start_draw_y + (this->m_height / 2) + (metrics.height / 2) - 3;

            // Draw the text
            gc.setDrawFont(font.family.c_str(), font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
            gc.setColor(text_color);
            gc.drawText(text_x, text_y, m_text.c_str());
        }

        void set_text(std::string text)
        {
            m_text = std::move(text);
        }
        const std::string &text() const
        {
            return m_text;
        }

    private:
        std::string m_text;
    };

} // namespace horizon