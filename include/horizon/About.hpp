#pragma once
#include <string>
#include <vector>

namespace horizon
{
    struct SomeoneInfo
    {
        std::string name;
        std::string url;
        std::string email;
    };

    struct About
    {
        std::string title;
        std::string description;
        std::string web;
        std::string git;
        std::string version;
        std::string icon;
        std::vector<SomeoneInfo> authors;
        std::vector<SomeoneInfo> translators;
    };

    const About ABOUT_HORIZON = {
        "Horizon",
        "Horizon is a free and open-source desktop environment for Linux.",
        "https://github.com/austral-os/horizon",
        "https://github.com/austral-os/horizon",
        "0.1.0",
        "horizon-desktop",
        {{"Horacio Daniel Ros", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com"}},
        {{"Horacio", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com"}}};
} // namespace horizon