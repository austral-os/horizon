#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Notebook.hpp>
#include <views/WifiView/WifiConfigView.hpp>

namespace horizon::preferences
{
    class WifiView : public Widget
    {
    public:
        WifiView();
        ~WifiView() override = default;
    private:
        Notebook* m_notebook{nullptr};
        WifiConfigView* m_wifi_config{nullptr};
    };
}
