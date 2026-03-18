#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class PrintersView : public Widget
    {
    public:
        PrintersView();
        ~PrintersView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
