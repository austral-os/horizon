#include <horizon/dialogs/PrintDialog.hpp>
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <iostream>
#include <thread>

namespace horizon {

PrintDialog::PrintDialog(Widget* print_target)
    : m_print_target(print_target)
{
    set_name("PrintDialog");
    set_resizable(false);
    
    m_backend = std::make_shared<horizon::print::backends::CUPSBackend>();
    setup_ui();
    load_printers();
}

PrintDialog::~PrintDialog() = default;

void PrintDialog::setup_ui()
{
    auto win = std::make_unique<Window>("Print Document");
    win->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    win->set_margin(20);
    win->set_spacing(15);
    win->set_background_color(Color("#ffffff"));
    win->set_size(500, 400);

    auto header = std::make_unique<Label>();
    header->set_text("Imprimir Documento");
    header->set_font_size(24);
    header->set_font_weight(FONT_WEIGHT_BOLD);
    win->add_child(std::move(header));

    auto desc = std::make_unique<Label>();
    desc->set_text("Selecciona una impresora:");
    win->add_child(std::move(desc));

    auto table = std::make_unique<TableView<horizon::print::Printer>>();
    table->set_position_type(FILL);
    
    TableColumn<horizon::print::Printer> col_name;
    col_name.id = "name";
    col_name.title = "Nombre";
    col_name.width = 150;
    col_name.cell_factory = [](const horizon::print::Printer& p) {
        auto lbl = std::make_unique<Label>();
        lbl->set_text(p.name);
        return lbl;
    };
    table->add_column(std::move(col_name));

    TableColumn<horizon::print::Printer> col_uri;
    col_uri.id = "uri";
    col_uri.title = "URI";
    col_uri.width = 250;
    col_uri.cell_factory = [](const horizon::print::Printer& p) {
        auto lbl = std::make_unique<Label>();
        lbl->set_text(p.uri);
        return lbl;
    };
    table->add_column(std::move(col_uri));
    
    m_printer_list = table.get();
    
    table->when_row_click.connect([this](const TableViewRowMouseClickContext<horizon::print::Printer>& ctx) {
        int index = ctx.row_index;
        if (index >= 0 && index < (int)m_printers.size()) {
            m_selected_printer = m_printers[index];
            m_print_btn->set_enabled(true);
        } else {
            m_selected_printer = std::nullopt;
            m_print_btn->set_enabled(false);
        }
    });

    win->add_child(std::move(table));

    auto btn_row = std::make_unique<Widget>();
    btn_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    btn_row->set_fixed_size(40);
    btn_row->set_spacing(10);

    auto spacer = std::make_unique<Widget>();
    spacer->set_position_type(FILL);
    btn_row->add_child(std::move(spacer));

    auto cancel_btn = std::make_unique<Button<SolidObject>>();
    cancel_btn->set_text("Cancelar");
    cancel_btn->set_fixed_size(100);
    m_cancel_btn = cancel_btn.get();
    m_cancel_btn->when_click.connect([this](auto&) {
        this->quit();
    });
    btn_row->add_child(std::move(cancel_btn));

    auto print_btn = std::make_unique<Button<SolidObject>>();
    print_btn->set_text("Imprimir");
    print_btn->set_fixed_size(100);
    print_btn->set_enabled(false);
    m_print_btn = print_btn.get();
    m_print_btn->when_click.connect([this](auto&) {
        this->print();
    });
    btn_row->add_child(std::move(print_btn));

    win->add_child(std::move(btn_row));
    set_root(std::move(win));
}

void PrintDialog::load_printers()
{
    m_printers.clear();
    
    std::thread([this]() {
        auto printers = m_backend->listPrinters();
        
        this->post_task([this, printers]() {
            m_printers = printers;
            m_printer_list->set_data(m_printers);
        });
    }).detach();
}

void PrintDialog::print()
{
    if (!m_selected_printer || !m_print_target) return;
    
    m_print_btn->set_enabled(false);
    m_print_btn->set_text("Enviando...");

    horizon::print::PrintConfig config;
    auto target = m_print_target;
    std::string printer_id = m_selected_printer->id;
    auto backend = m_backend;

    std::thread([this, target, printer_id, backend, config]() {
        auto doc = target->generate_print_document(config);
        
        if (doc.isValid()) {
            auto job_id = backend->submitJob(printer_id, doc, config);
            if (!job_id.empty()) {
                std::cout << "Print job submitted successfully: " << job_id << std::endl;
                this->post_task([this]() {
                    this->quit();
                });
                return;
            }
        }
        
        this->post_task([this]() {
            m_print_btn->set_text("Error");
            m_print_btn->set_enabled(true);
        });
    }).detach();
}

} // namespace horizon
