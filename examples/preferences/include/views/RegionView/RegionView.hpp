#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class RegionView : public Widget
    {
    public:
        RegionView();
        ~RegionView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
