#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class AppearanceView : public Widget
    {
    public:
        AppearanceView();
        ~AppearanceView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
