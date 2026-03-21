#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <views/DisplayView/DisplayDevices.hpp>

namespace horizon::preferences
{
    class DisplayView : public Widget
    {
    public:
        DisplayView();
        ~DisplayView() override = default;
    private:
        Label* m_title_label{nullptr};
        DisplayDevices* m_display_devices{nullptr};
    };
}
