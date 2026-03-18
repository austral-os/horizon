#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class SoundView : public Widget
    {
    public:
        SoundView();
        ~SoundView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
