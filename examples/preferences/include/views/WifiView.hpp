#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class WifiView : public Widget
    {
    public:
        WifiView();
        ~WifiView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
