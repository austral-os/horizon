#pragma once

#include <horizon/Button.hpp>
#include <horizon/Color.hpp>
#include <horizon/ColorArea2D.hpp>
#include <horizon/GradientBar.hpp>
#include <horizon/Label.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{
    /**
     * @class ColorPicker
     * @brief A professional-grade color picker widget.
     *
     * Features:
     * - HSV area and Hue bar.
     * - RGB, HSV, LCh component sliders.
     * - HTML notation input.
     * - Current/Previous color preview.
     * - Palette selection.
     */
    class ColorPicker : public Widget
    {
    public:
        ColorPicker();
        virtual ~ColorPicker();

        void set_color(const Color &color);
        Color color() const
        {
            return m_color;
        }

        EventsManager when_color_changed;

    private:
        void setup_layout();
        void update_ui_from_color();
        void update_color_from_ui();

        Color m_color;
        Color m_previous_color;

        // Components
        ColorArea2D *m_area2d_sv;
        GradientBar *m_hue_bar;

        // RGB Sliders
        GradientBar *m_r_slider, *m_g_slider, *m_b_slider;
        TextBox *m_r_box, *m_g_box, *m_b_box;

        // HSV Sliders (can reuse LCh/etc based on logic)
        GradientBar *m_h_slider, *m_s_slider, *m_v_slider;
        TextBox *m_h_box, *m_s_box, *m_v_box;

        TextBox *m_hex_box;
        Widget *m_active_input = nullptr;

        // Preview widget (custom inner widget or just a colored frame)
        class ColorPreview : public Widget
        {
        public:
            Color current;
            Color previous;
            void draw(GraphicsContext &gc) override;
        };
        ColorPreview *m_preview;
    };
} // namespace horizon
