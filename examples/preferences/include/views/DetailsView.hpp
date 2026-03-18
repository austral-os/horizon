#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class DetailsView : public Widget
    {
    public:
        DetailsView();
        ~DetailsView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
