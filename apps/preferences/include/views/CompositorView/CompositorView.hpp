#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Checkbox.hpp>

namespace horizon::preferences
{
    class CompositorView : public Widget
    {
    public:
        CompositorView();
        ~CompositorView() override = default;

    private:
        Label* m_title_label{nullptr};

        Checkbox<AquaObject>* m_minimize_cb{nullptr};
        Checkbox<AquaObject>* m_open_close_cb{nullptr};
        Checkbox<AquaObject>* m_shadows_cb{nullptr};
        Checkbox<AquaObject>* m_wobbly_cb{nullptr};
        Checkbox<AquaObject>* m_blur_cb{nullptr};

        void setup_checkbox(Checkbox<AquaObject>*& cb, const std::string& label_text, const std::string& plugin_name);
    };
}
