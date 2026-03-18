#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class ScreensaverView : public Widget
    {
    public:
        ScreensaverView();
        ~ScreensaverView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
