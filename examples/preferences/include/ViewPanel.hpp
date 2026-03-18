#pragma once

#include <horizon/GroupedIconsView.hpp>

namespace horizon::preferences
{
    class ViewPanel : public GroupedIconsView
    {
    public:
        ViewPanel();
        ~ViewPanel() override = default;
    };
} // namespace horizon::preferences
