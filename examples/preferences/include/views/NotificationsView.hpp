#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class NotificationsView : public Widget
    {
    public:
        NotificationsView();
        ~NotificationsView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
