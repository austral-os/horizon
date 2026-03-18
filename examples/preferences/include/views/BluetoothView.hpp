#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class BluetoothView : public Widget
    {
    public:
        BluetoothView();
        ~BluetoothView() override = default;
    private:
        Label* m_title_label{nullptr};
    };
}
