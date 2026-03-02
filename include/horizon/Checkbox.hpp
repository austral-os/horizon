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
     * @brief A Checkbox widget that can be styled as SolidObject or AquaObject.
     * @tparam T The base class (SolidObject or AquaObject).
     */
    template <typename T> class Checkbox : public T
    {
    public:
        Checkbox() : T()
        {
            m_label = std::make_unique<Label>();

            this->when_mouse_enter.connect([this](EventContext &) { this->invalidate(); });
            this->when_mouse_leave.connect([this](EventContext &) { this->invalidate(); });

            this->when_mouse_press.connect(
                [this](EventContext &ctx)
                {
                    if (ctx.button == 0x110)
                    {
                        m_checked = !m_checked;
                        this->invalidate();
                        if (m_on_toggle)
                            m_on_toggle(m_checked);
                    }
                });

            this->set_fixed_size(36);
        }
        ~Checkbox() = default;

        void draw(GraphicsContext &gc) override
        {
            auto *tm = this->application()->theme_manager.get();
            Color window_fg = tm->get_color("window_fg");

            int marker_size = 22;
            int margin_y = (this->m_height - marker_size);
            int marker_x = this->m_start_draw_x + 10;
            int marker_y = this->m_start_draw_y + margin_y;

            // Update accent color based on state
            this->set_accent_color(m_checked ? WidgetAccentColor::Primary
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
            this->m_x = marker_x;
            this->m_y = marker_y;
            this->m_start_draw_x = marker_x;
            this->m_start_draw_y = marker_y;
            this->m_width = marker_size;
            this->m_height = marker_size;

            // Use a fixed small radius for the checkbox square
            CornerRadius old_radius = this->corner_radius();
            this->set_corner_radius({4, 4, 4, 4});

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

            if (m_checked)
            {
                gc.setColor(window_fg);

                // Draw a checkmark using the new drawLine method
                // Relative points in the 16x16 marker:
                // Start: (2, 8) -> Mid: (7, 13) -> End: (14, 2)
                int x1 = marker_x + 4;
                int y1 = marker_y + 8;
                int x2 = marker_x + 9;
                int y2 = marker_y + 16;
                int x3 = marker_x + 18;
                int y3 = marker_y + 2;

                gc.drawLine(x1, y1, x2, y2, 3.0f);
                gc.drawLine(x2, y2, x3, y3, 2.0f);
            }

            // Draw label
            int label_x = marker_x + marker_size + 12;
            int label_w = this->m_width - (label_x - this->m_x) - 10;

            m_label->set_application_recursive(this->application());
            m_label->set_position(label_x, this->m_y);
            m_label->set_size(label_w, this->m_height);
            m_label->set_vertical_alignment(VerticalAlignment::Middle);
            m_label->calculate_layout();
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

        void set_checked(bool checked)
        {
            m_checked = checked;
            this->invalidate();
        }
        bool is_checked() const
        {
            return m_checked;
        }

        void set_on_toggle(std::function<void(bool)> handler)
        {
            m_on_toggle = std::move(handler);
        }

        Label *label()
        {
            return m_label.get();
        }

    private:
        std::unique_ptr<Label> m_label;
        bool m_checked{false};
        std::function<void(bool)> m_on_toggle;
    };

} // namespace horizon
