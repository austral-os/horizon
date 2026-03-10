#pragma once

#include <horizon/ApplicationWindow.hpp>

namespace horizon::arkfm
{
    class ArkfmWindow : public ApplicationWindow
    {
    public:
        ArkfmWindow(int w, int h);
        ~ArkfmWindow() override = default;
    };
} // namespace horizon::arkfm