#pragma once
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/Slider.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Widget.hpp>
#include <horizon/ConfigManager.hpp>
#include <memory>
#include <utils/PipeWireManager.hpp>

namespace horizon::preferences
{
    class SoundView : public Widget
    {
    public:
        SoundView();
        ~SoundView() override;

    private:
        void setup_ui();
        void setup_output_tab(Widget *container);
        void setup_input_tab(Widget *container);
        void update_device_list();
        void on_volume_slider_changed(float value);
        void on_balance_slider_changed(float value);
        void test_speakers();
        void save_sound_config();

        Notebook *m_notebook{nullptr};

        // Output tab elements
        TableView<AudioItem> *m_output_table{nullptr};
        Slider *m_output_volume_slider{nullptr};
        Slider *m_balance_slider{nullptr};
        Button<AquaObject> *m_test_btn{nullptr};

        // Input tab elements
        TableView<AudioItem> *m_input_table{nullptr};
        Slider *m_input_volume_slider{nullptr};

        // Apps tab elements
        void setup_apps_tab(Widget *container);
        TableView<AudioItem> *m_apps_table{nullptr};

        Label *m_title_label{nullptr};
        std::unique_ptr<ConfigManager> m_config;
    };
} // namespace horizon::preferences
