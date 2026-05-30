#pragma once

#include <horizon/WaylandWindow.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Button.hpp>
#include <horizon/TableView.hpp>
#include <horizon/print/Models.h>
#include <horizon/print/backends/CUPSBackend.h>
#include <memory>

namespace horizon {

class PrintDialog : public WaylandWindow {
public:
    PrintDialog(Widget* print_target);
    ~PrintDialog() override;

private:
    void setup_ui();
    void load_printers();
    void print();

    Widget* m_print_target{nullptr};
    std::shared_ptr<horizon::print::backends::CUPSBackend> m_backend;
    
    std::vector<horizon::print::Printer> m_printers;
    std::optional<horizon::print::Printer> m_selected_printer;

    TableView<horizon::print::Printer>* m_printer_list{nullptr};
    Button<SolidObject>* m_print_btn{nullptr};
    Button<SolidObject>* m_cancel_btn{nullptr};
};

} // namespace horizon
