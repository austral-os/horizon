#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/LoadingBar.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/print/PrinterDiscovery.h>
#include <horizon/print/Models.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace horizon::preferences
{
    class AddPrinterDialog : public WaylandWindow
    {
    public:
        AddPrinterDialog();
        ~AddPrinterDialog() override;

        // Evento que se dispara cuando el usuario presiona "Agregar"
        EventsManager<horizon::print::Printer> when_accepted;

    private:
        void setup_ui();
        void start_scanning();
        void stop_scanning();
        void filter_printers(const std::string& query);
        void on_printer_selected(const horizon::print::Printer& printer);
        void on_add_clicked();

        TextBox<TextPolicy>* m_search_box{nullptr};
        TableView<horizon::print::Printer>* m_table_view{nullptr};
        LoadingBar* m_loading_bar{nullptr};
        Label* m_status_label{nullptr};
        Button<AquaObject>* m_add_btn{nullptr};
        Button<AquaObject>* m_cancel_btn{nullptr};

        std::unique_ptr<horizon::print::PrinterDiscovery> m_discovery;
        std::vector<horizon::print::Printer> m_discovered_printers;
        horizon::print::Printer m_selected_printer;
        std::mutex m_printers_mutex;
    };
}
