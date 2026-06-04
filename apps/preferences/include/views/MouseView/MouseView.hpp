#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Slider.hpp>
#include <horizon/ConfigManager.hpp>
#include <memory>

namespace horizon::preferences
{
    class MouseView : public Widget
    {
    public:
        MouseView();
        ~MouseView() override = default;

    private:
        void setup_ui();
        void load_config();
        void save_config();
        void apply_to_labwc(float speed);
        void apply_to_meteor(float speed);

        Label* m_title_label{nullptr};
        Slider* m_double_click_slider{nullptr};
        Slider* m_pointer_speed_slider{nullptr};
        std::unique_ptr<ConfigManager> m_config;
    };
}
