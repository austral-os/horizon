#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class MouseView : public Widget
    {
    public:
        MouseView();
        ~MouseView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
