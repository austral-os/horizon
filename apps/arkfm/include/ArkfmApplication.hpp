#pragma once

#include <horizon/Application.hpp>

namespace horizon::arkfm
{

    class ArkfmApplication : public Application
    {
    public:
        ArkfmApplication();
        ~ArkfmApplication() override = default;
    };

} // namespace horizon::arkfm