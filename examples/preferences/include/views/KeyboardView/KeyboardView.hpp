#pragma once
#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/Widget.hpp>

namespace horizon::preferences
{
    class KeyboardView : public Widget
    {
    public:
        KeyboardView();
        ~KeyboardView() override = default;

    private:
        Notebook *m_notebook{nullptr};
    };
} // namespace horizon::preferences
