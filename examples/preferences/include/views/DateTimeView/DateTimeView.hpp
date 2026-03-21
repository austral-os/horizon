#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class DateTimeView : public Widget
    {
    public:
        DateTimeView();
        ~DateTimeView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
