#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Slider.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Label.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Combo.hpp>

namespace horizon::preferences
{
    class KeyboardHardwareView : public Widget
    {
    public:
        KeyboardHardwareView();
        ~KeyboardHardwareView() override = default;

    private:
        Slider* m_delay_slider{nullptr};
        Slider* m_rate_slider{nullptr};
        Checkbox<AquaObject>* m_numlock_checkbox{nullptr};
        Combo* m_model_combo{nullptr};
    };
} // namespace horizon::preferences
