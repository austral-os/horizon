#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/LoadingBar.hpp>
#include <horizon/EventsManager.hpp>
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace horizon::preferences
{
    struct DriverPackage {
        std::string name;
        std::string description;
    };

    class DriverSearchDialog : public WaylandWindow
    {
    public:
        DriverSearchDialog(const std::string& printer_name);
        ~DriverSearchDialog() override;

        // Disparado cuando el driver se instaló con apt, o cuando se seleccionó un PPD manual (pasando la ruta)
        EventsManager<std::string> when_driver_ready;

    private:
        void setup_ui();
        void start_search();
        void install_selected();

        std::string m_printer_name;
        std::vector<DriverPackage> m_packages;
        std::optional<DriverPackage> m_selected_package;

        Label* m_status_label{nullptr};
        LoadingBar* m_loading_bar{nullptr};
        TableView<DriverPackage>* m_table_view{nullptr};
        Button<AquaObject>* m_install_btn{nullptr};
        Button<AquaObject>* m_manual_btn{nullptr};
        Button<AquaObject>* m_cancel_btn{nullptr};
    };
}
