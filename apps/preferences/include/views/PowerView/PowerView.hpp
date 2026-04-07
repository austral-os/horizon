#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class PowerView : public Widget
    {
    public:
        PowerView();
        ~PowerView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
