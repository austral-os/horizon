#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class DisplayView : public Widget
    {
    public:
        DisplayView();
        ~DisplayView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
