#pragma once

#include "horizon/Widget.hpp"
#include "horizon/Button.hpp"
#include "horizon/SolidObject.hpp"
#include "horizon/Color.hpp"
#include "horizon/dialogs/ColorPickerDialog.hpp"
#include <memory>

namespace horizon
{
    class ColorSelector : public Widget
    {
    public:
        ColorSelector();
        ~ColorSelector() override = default;

        const Color &color() const { return m_color; }
        void set_color(const Color &color);

        EventsManager<ColorPickerDialogAcceptedContext> when_color_changed;

    private:
        class ColorBox : public Widget
        {
        public:
            Color color;
            void draw(GraphicsContext &gc) override;
            void calculate_layout() override;
        };

        ColorBox *m_preview_ptr;
        std::unique_ptr<Button<SolidObject>> m_button;
        Color m_color;
    };
} // namespace horizon
