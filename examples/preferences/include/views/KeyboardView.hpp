#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class KeyboardView : public Widget
    {
    public:
        KeyboardView();
        ~KeyboardView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
