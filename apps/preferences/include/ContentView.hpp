#pragma once

#include <horizon/Widget.hpp>
#include <memory>

namespace horizon::preferences
{
    class ContentView : public Widget
    {
    public:
        ContentView();
        ~ContentView() override = default;

        void load_view(std::unique_ptr<Widget> view);
    };
} // namespace horizon::preferences
