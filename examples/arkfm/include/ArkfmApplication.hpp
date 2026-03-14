#pragma once

#include <horizon/WaylandWindow.hpp>

namespace horizon::arkfm
{

    class ArkfmApplication : public WaylandWindow
    {
    public:
        ArkfmApplication();
        ~ArkfmApplication() override = default;
    };

} // namespace horizon::arkfm