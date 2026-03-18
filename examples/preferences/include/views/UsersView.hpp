#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class UsersView : public Widget
    {
    public:
        UsersView();
        ~UsersView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
