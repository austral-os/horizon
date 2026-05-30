#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Button.hpp>
#include <horizon/print/Models.h>
#include <horizon/print/PrinterService.h>
#include <horizon/print/AsyncPrintService.h>
#include <memory>

namespace horizon::preferences
{
    class PrintersView : public Widget
    {
    public:
        PrintersView();
        ~PrintersView() override;

    private:
        void setup_ui();
        void refresh_printers();
        void on_printer_selected(const horizon::print::Printer &printer);

        // UI Components
        TableView<horizon::print::Printer>* m_printer_table{nullptr};
        Widget* m_details_container{nullptr};
        Label* m_name_label{nullptr};
        Label* m_uri_label{nullptr};
        Label* m_source_label{nullptr};
        Button<AquaObject>* m_test_page_btn{nullptr};

        // Services
        std::shared_ptr<horizon::print::IPrintBackend> m_backend;
        std::unique_ptr<horizon::print::PrinterService> m_printer_service;
        std::unique_ptr<horizon::print::AsyncPrintService> m_async_service;

        // State
        std::optional<horizon::print::Printer> m_selected_printer;
    };
}
