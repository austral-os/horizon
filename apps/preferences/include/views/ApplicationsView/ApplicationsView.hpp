#pragma once

#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/Widget.hpp>

namespace horizon::preferences
{
    class ApplicationsView : public Widget
    {
    public:
        ApplicationsView();
        ~ApplicationsView() override = default;

    private:
        Notebook *m_notebook{nullptr};
    };
} // namespace horizon::preferences
