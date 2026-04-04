#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <vector>
#include <string>

namespace horizon::preferences
{
    struct WifiNetwork
    {
        std::string ssid;
        std::string security;
    };

    class WifiConfigView : public Widget
    {
    public:
        WifiConfigView();
        ~WifiConfigView() override = default;

        void refresh_networks();

    private:
        void setup_ui();
        std::vector<WifiNetwork> scan_networks();

        Label* m_title_label{nullptr};
        TableView<WifiNetwork>* m_table_view{nullptr};
        Button<AquaObject>* m_add_button{nullptr};
        Button<AquaObject>* m_remove_button{nullptr};
        Checkbox<AquaObject>* m_remember_checkbox{nullptr};
    };
}
