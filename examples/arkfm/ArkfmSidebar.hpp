#pragma once

#include "horizon/Sidebar.hpp"
namespace horizon::arkfm
{
    class ArkfmSidebar : public Sidebar
    {
    public:
        ArkfmSidebar();
        ~ArkfmSidebar() override = default;
    };
} // namespace horizon::arkfm