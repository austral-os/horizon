#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class NetworkView : public Widget
    {
    public:
        NetworkView();
        ~NetworkView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
