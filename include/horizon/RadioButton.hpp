#pragma once
#include "horizon/AquaObject.hpp"
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Label.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    /**
     * @brief A RadioButton widget that can be styled as SolidObject or AquaObject.
     * @tparam T The base class (SolidObject or AquaObject).
     */
    template <typename T> class RadioButton : public T
    {
    public:
        RadioButton() : T()
        {
            m_label = std::make_unique<Label>();

            this->when_mouse_enter.connect([this](EventContext &) { this->invalidate(); });
            this->when_mouse_leave.connect([this](EventContext &) { this->invalidate(); });

            this->when_mouse_press.connect(
                [this](EventContext &ctx)
                {
                    if (ctx.button == 0x110 && !m_selected)
                    {
                        m_selected = true;
                        this->invalidate();
                        if (m_on_select)
                            m_on_select();
                    }
                });

            this->set_fixed_size(36);
        }
        ~RadioButton() = default;

        void draw(GraphicsContext &gc) override
        {
            auto *tm = this->application()->theme_manager.get();
            Color window_fg = tm->get_color("window_fg");

            int marker_radius = 12;
            int margin_y = (this->m_height) / 2;
            int marker_x = this->m_start_draw_x + 10 + marker_radius;
            int marker_y = this->m_start_draw_y + margin_y;

            // Update accent color based on state
            this->set_accent_color(m_selected ? WidgetAccentColor::Primary
                                              : WidgetAccentColor::Default);

            // --- Temporary Geometry Override ---
            // We save original values so T::draw (Aqua/Solid) only paints the marker
            int old_x = this->m_x;
            int old_y = this->m_y;
            int old_sdx = this->m_start_draw_x;
            int old_sdy = this->m_start_draw_y;
            int old_w = this->m_width;
            int old_h = this->m_height;

            // We treat the marker as a square area for T::draw
            int marker_size = marker_radius * 2;
            this->m_x = marker_x - marker_radius;
            this->m_y = marker_y - marker_radius;
            this->m_start_draw_x = marker_x - marker_radius;
            this->m_start_draw_y = marker_y - marker_radius;
            this->m_width = marker_size;
            this->m_height = marker_size;

            // We need to set a circular corner radius so T::draw (Aqua/Solid) draws it round
            CornerRadius old_radius = this->corner_radius();
            this->set_corner_radius({marker_radius, marker_radius, marker_radius, marker_radius});

            // Draw the themed background ONLY for the marker
            T::draw(gc);

            // Restore geometry and radius
            this->m_x = old_x;
            this->m_y = old_y;
            this->m_start_draw_x = old_sdx;
            this->m_start_draw_y = old_sdy;
            this->m_width = old_w;
            this->m_height = old_h;
            this->set_corner_radius(old_radius);

            if (m_selected)
            {
                gc.setColor(window_fg);
                // Draw a simple inner circle for selection
                gc.fillCircle(marker_x, marker_y, marker_radius / 3);
            }

            // Draw label
            int label_x = marker_x + marker_radius + 12;
            int label_w = this->m_width - (label_x - this->m_x) - 10;

            m_label->set_application_recursive(this->application());
            m_label->set_position(label_x, this->m_y + (this->m_height / 2) - marker_radius);
            m_label->set_size(label_w, this->m_height);
            m_label->draw(gc);
        }

        void set_text(std::string text)
        {
            m_label->set_text(std::move(text));
        }
        const std::string &text() const
        {
            return m_label->text();
        }

        void set_selected(bool selected)
        {
            m_selected = selected;
            this->invalidate();
        }
        bool is_selected() const
        {
            return m_selected;
        }

        void set_on_select(std::function<void()> handler)
        {
            m_on_select = std::move(handler);
        }

        Label *label()
        {
            return m_label.get();
        }

    private:
        std::unique_ptr<Label> m_label;
        bool m_selected{false};
        std::function<void()> m_on_select;
    };

} // namespace horizon
