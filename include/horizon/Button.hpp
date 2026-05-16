#pragma once
#include "horizon/AirObject.hpp"
#include "horizon/AquaObject.hpp"
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Label.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{

    template <typename T> class Button : public T
    {
    public:
        Button() : T()
        {
            this->set_focusable(true);
            // this->when_mouse_enter.connect([this](EventContext &) { this->invalidate(); });
            // this->when_mouse_leave.connect([this](EventContext &) { this->invalidate(); });

            if constexpr (std::is_same_v<T, AquaObject>)
            {
                this->set_corner_radius({15, 15, 15, 15});
            }
            else if constexpr (std::is_same_v<T, SolidObject>)
            {
                this->set_corner_radius({6, 6, 6, 6});
            }
            else if constexpr (std::is_same_v<T, AirObject>)
            {
                this->set_corner_radius({8, 8, 8, 8});
            }

            m_label = std::make_unique<Label>();
            m_label->set_alignment(TextAlignment::Center);
        }
        ~Button() = default;

        void set_size(int width, int height)
        {
            T::set_size(width, height);
            m_label->set_size(width - 10, height - 10);
            this->invalidate();
        }

        void draw(GraphicsContext &gc) override
        {
            auto *tm = theme_manager();
            auto theme_font = tm->get_font("window");

            std::string family = theme_font.family;
            int size = theme_font.size;

            T::draw(gc);

            // Center the label within the button, accounting for children (like icons)
            int margin = 5;
            int label_offset = 0;

            // If we have children (e.g. an icon at the beginning),
            // we should offset the label to the right.
            if (!this->m_children.empty())
            {
                // Approximate width of children + spacing
                // For a 16px icon + 4px spacing, we offset by 20.
                label_offset = 24;
            }

            m_label->set_application_recursive(this->application());
            m_label->set_position(this->m_start_draw_x + margin + label_offset,
                                  this->m_start_draw_y + margin);
            m_label->set_size(this->m_width - (margin * 2) - label_offset,
                              this->m_height - (margin * 2) - 3);
            m_label->set_alignment(label_offset > 0 ? TextAlignment::Left : TextAlignment::Center);
            m_label->calculate_layout();

            if (T::m_draw_state == WidgetDrawState::PRESSED)
            {
                m_label->set_font_size(size - 1);
            }
            else
            {
                m_label->set_font_size(size);
            }

            m_label->draw(gc);
        }

        void set_text(std::string text)
        {
            m_label->set_text(std::move(text));
        }

        void set_font_weight(FontWeight weight)
        {
            m_label->set_font_weight(weight);
        }

        const std::string &text() const
        {
            return m_label->text();
        }

        void set_text_color(Color color)
        {
            m_label->set_text_color(color);
        }

    private:
        std::unique_ptr<Label> m_label;
    };

} // namespace horizon