#pragma once
#include <string>
#include <vector>

namespace horizon::preferences
{
    struct KeyboardModel
    {
        std::string id;
        std::string description;
        std::string vendor;
    };

    struct KeyboardLayout
    {
        std::string id;
        std::string description;
    };

    class XkbParser
    {
    public:
        static std::vector<KeyboardModel> get_models();
        static std::vector<KeyboardLayout> get_layouts();
    };
} // namespace horizon::preferences
