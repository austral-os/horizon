#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class DetailsView : public horizon::Widget
    {
    public:
        DetailsView();
        ~DetailsView() override = default;
    private:
        horizon::Label* m_title_label{nullptr};
    };
}
