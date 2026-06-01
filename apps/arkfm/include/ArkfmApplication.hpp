#pragma once

#include <horizon/Application.hpp>

namespace horizon::arkfm
{

    class ArkfmApplication : public Application
    {
    public:
        ArkfmApplication(const std::string& initial_path = "");
        ~ArkfmApplication() override = default;
    };

} // namespace horizon::arkfm