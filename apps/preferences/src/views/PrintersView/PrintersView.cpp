#include "views/PrintersView/DriverSearchDialog.hpp"
#include <views/PrintersView/PrintersView.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Frame.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/print/backends/CUPSBackend.h>
#include <views/PrintersView/AddPrinterDialog.hpp>
#include <iostream>

namespace horizon::preferences
{
    PrintersView::PrintersView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_spacing(20);
        set_margin(20);

        // Initialize Services
        m_backend = std::make_shared<horizon::print::backends::CUPSBackend>();
        m_printer_service = std::make_unique<horizon::print::PrinterService>(m_backend);
        
        // Creamos un PrintService dummy para el AsyncService
        auto print_service = std::make_shared<horizon::print::PrintService>(m_backend);
        // Aunque PrintService lo requiera compartido, PrinterService no, así que 
        // usaremos el sincrono para listar o creamos un AsyncPrintService.
        // Por simplicidad, y porque listPrinters en CUPSBackend es bastante rápido,
        // usaremos m_printer_service directamente de forma síncrona aquí para listar,
        // o podríamos configurar el AsyncService completo.
        
        setup_ui();
        refresh_printers();
    }

    PrintersView::~PrintersView()
    {
    }

    void PrintersView::setup_ui()
    {
        const int left_panel_width = 230;

        // --- Left Panel: TableView ---
        auto left_panel = std::make_unique<Widget>();
        left_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_panel->set_fixed_size(left_panel_width);
        left_panel->set_spacing(10);

        auto table = std::make_unique<TableView<horizon::print::Printer>>();
        table->set_header_visible(false);
        table->set_row_height(60);
        table->set_background_color(Color("#f0f0f0"));

        TableColumn<horizon::print::Printer> col;
        col.width = left_panel_width;
        col.cell_factory = [](const horizon::print::Printer &data)
        {
            auto container = std::make_unique<Widget>();
            container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            container->set_spacing(12);
            container->set_margin(8);

            // Icon
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name("printer");
            icon->set_fixed_size(32);
            container->add_child(std::move(icon));

            // Info Container
            auto info = std::make_unique<Widget>();
            info->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            info->set_spacing(2);

            auto title = std::make_unique<Label>(data.name);
            title->set_font_weight(FONT_WEIGHT_BOLD);
            info->add_child(std::move(title));

            auto subtitle = std::make_unique<Label>(data.source == horizon::print::PrinterSource::Installed ? "Installed" : "Discovered");
            subtitle->set_text_color(Color("#666666"));
            subtitle->set_font_size(14);
            info->add_child(std::move(subtitle));

            container->add_child(std::move(info));

            return container;
        };
        table->add_column(std::move(col));

        m_printer_table = table.get();
        m_printer_table->when_row_click.connect([this](auto &ctx)
                                               { this->on_printer_selected(ctx.row_data); });

        left_panel->add_child(std::move(table));

        // Bottom buttons (+ / -)
        auto toolbar = std::make_unique<Widget>();
        toolbar->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        toolbar->set_fixed_size(30);
        toolbar->set_spacing(2);

        auto add_btn = std::make_unique<Button<SolidObject>>();
        add_btn->set_text("+");
        add_btn->set_fixed_size(30);
        
        add_btn->when_click.connect([this](MouseButtonEventContext &) {
            auto dialog = std::make_unique<AddPrinterDialog>();
            dialog->when_accepted.connect([this](horizon::print::Printer& ev) {
                if (auto* app = this->application()) {
                    std::thread([this, app, ev]() {
                        try {
                            horizon::print::PrintConfig config;
                            this->m_printer_service->addPrinter(ev.name, ev.uri, config);
                            
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));

                            app->post_task([this]() {
                                this->refresh_printers();
                                this->invalidate();
                            });
                        } catch (const std::exception& e) {
                            std::string err_msg = e.what();
                            if (err_msg == "DRIVER_MISSING") {
                                app->post_task([this, app, ev]() {
                                    auto search_dialog = std::make_unique<DriverSearchDialog>(ev.name);
                                    search_dialog->when_driver_ready.connect([this, app, ev](std::string ppd_path) {
                                        std::thread([this, app, ev, ppd_path]() {
                                            try {
                                                horizon::print::PrintConfig config;
                                                config.ppdPath = ppd_path;
                                                this->m_printer_service->addPrinter(ev.name, ev.uri, config);
                                                
                                                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                                app->post_task([this]() {
                                                    this->refresh_printers();
                                                    this->invalidate();
                                                });
                                            } catch (const std::exception& e2) {
                                                std::cerr << "Error final adding printer: " << e2.what() << "\n";
                                            }
                                        }).detach();
                                    });
                                    auto* raw_app = app;
                                    std::thread([raw_app, d = std::move(search_dialog)]() mutable {
                                        d->initialize();
                                        d->run();
                                        if (raw_app) raw_app->wakeup();
                                    }).detach();
                                });
                            } else {
                                std::cerr << "Error adding printer: " << err_msg << "\n";
                            }
                        }
                    }).detach();
                }
            });
            // Show the dialog in a detached thread to prevent blocking the main window
            auto* app = this->application();
            std::thread([app, d = std::move(dialog)]() mutable {
                d->initialize();
                d->run();
                if (app) app->wakeup();
            }).detach();
        });
        
        toolbar->add_child(std::move(add_btn));

        auto rem_btn = std::make_unique<Button<SolidObject>>();
        rem_btn->set_text("-");
        rem_btn->set_fixed_size(30);
        
        rem_btn->when_click.connect([this](MouseButtonEventContext &) {
            if (this->m_selected_printer.has_value()) {
                try {
                    this->m_printer_service->removePrinter(this->m_selected_printer->id);
                    this->refresh_printers();
                } catch (const std::exception& e) {
                    std::cerr << "Error removing printer: " << e.what() << "\n";
                }
            }
        });
        
        toolbar->add_child(std::move(rem_btn));

        left_panel->add_child(std::move(toolbar));
        add_child(std::move(left_panel));

        // --- Right Panel: Details Frame ---
        auto details = std::make_unique<Frame>();
        details->set_position_type(WidgetPositionTypes::FILL);
        details->set_margin(0);
        details->set_spacing(20);

        auto details_container = std::make_unique<Widget>();
        details_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        details_container->set_margin(20);
        details_container->set_spacing(20);

        auto create_row = [&](const std::string &label_text, Widget *field)
        {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(35);
            row->set_spacing(15);

            auto lbl = std::make_unique<Label>(label_text);
            lbl->set_alignment(TextAlignment::Right);
            lbl->set_width(120);
            row->add_child(std::move(lbl));

            std::unique_ptr<Widget> field_ptr(field);
            row->add_child(std::move(field_ptr));
            return row;
        };

        // Grid-like layout for info
        auto grid = std::make_unique<Widget>();
        grid->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        grid->set_spacing(15);

        auto name_lbl = std::make_unique<Label>("---");
        name_lbl->set_font_weight(FONT_WEIGHT_BOLD);
        m_name_label = name_lbl.get();
        grid->add_child(create_row("Printer Name:", name_lbl.release()));

        auto uri_lbl = std::make_unique<Label>("---");
        m_uri_label = uri_lbl.get();
        grid->add_child(create_row("Device URI:", uri_lbl.release()));

        auto source_lbl = std::make_unique<Label>("---");
        m_source_label = source_lbl.get();
        grid->add_child(create_row("Source:", source_lbl.release()));

        details_container->add_child(std::move(grid));

        // Test Page button
        auto bottom_row = std::make_unique<Widget>();
        bottom_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bottom_row->set_fixed_size(35);
        bottom_row->add_child(Spacer());
        
        auto test_btn = std::make_unique<Button<AquaObject>>();
        test_btn->set_text("Print Test Page");
        test_btn->set_fixed_size(150);
        test_btn->set_enabled(false); // Deshabilitado hasta seleccionar una
        
        test_btn->when_click.connect([this](MouseButtonEventContext &) {
            if (this->m_selected_printer.has_value()) {
                try {
                    // Generar un PDF mínimo válido para la prueba
                    horizon::print::PrintDocument doc;
                    std::string minimal_pdf = "%PDF-1.4\n1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>\nendobj\n4 0 obj\n<< /Length 53 >>\nstream\nBT\n/F1 24 Tf\n100 700 Td\n(Horizon Test Page) Tj\nET\nendstream\nendobj\n5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\nxref\n0 6\n0000000000 65535 f \n0000000009 00000 n \n0000000058 00000 n \n0000000115 00000 n \n0000000228 00000 n \n0000000332 00000 n \ntrailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n420\n%%EOF\n";
                    doc.data = std::vector<uint8_t>(minimal_pdf.begin(), minimal_pdf.end());
                    
                    horizon::print::PrintConfig config;
                    // Enviar directo al backend
                    this->m_backend->submitJob(this->m_selected_printer->id, doc, config);
                    
                    auto original_text = m_test_page_btn->text();
                    m_test_page_btn->set_text("Enviado!");
                    m_test_page_btn->set_enabled(false);
                    m_test_page_btn->invalidate();
                    
                    if (auto* app = this->application()) {
                        std::thread([this, app, original_text]() {
                            std::this_thread::sleep_for(std::chrono::seconds(2));
                            app->post_task([this, original_text]() {
                                if (this->m_test_page_btn) {
                                    this->m_test_page_btn->set_text(original_text);
                                    this->m_test_page_btn->set_enabled(true);
                                    this->m_test_page_btn->invalidate();
                                }
                            });
                        }).detach();
                    }
                    
                } catch (const std::exception& e) {
                    std::cerr << "Error printing test page: " << e.what() << "\n";
                    if (this->m_test_page_btn) {
                        this->m_test_page_btn->set_text("Error de Impresión");
                        this->m_test_page_btn->invalidate();
                    }
                }
            }
        });
        
        m_test_page_btn = test_btn.get();
        bottom_row->add_child(std::move(test_btn));

        details_container->add_child(std::move(bottom_row));

        m_details_container = details_container.get();
        details->add_child(std::move(details_container));
        add_child(std::move(details));
    }

    void PrintersView::refresh_printers()
    {
        if (m_printer_table && m_printer_service)
        {
            auto printers = m_printer_service->listPrinters();
            m_printer_table->set_data(printers);
            
            if (printers.empty()) {
                m_selected_printer = std::nullopt;
                if (m_name_label) m_name_label->set_text("---");
                if (m_uri_label) m_uri_label->set_text("---");
                if (m_source_label) m_source_label->set_text("---");
                if (m_test_page_btn) m_test_page_btn->set_enabled(false);
            }
        }
    }

    void PrintersView::on_printer_selected(const horizon::print::Printer &printer)
    {
        m_selected_printer = printer;
        
        if (m_name_label)
            m_name_label->set_text(printer.name);
            
        if (m_uri_label)
            m_uri_label->set_text(printer.uri);
            
        if (m_source_label)
            m_source_label->set_text(printer.source == horizon::print::PrinterSource::Installed ? "Installed" : "Discovered");
            
        if (m_test_page_btn) {
            m_test_page_btn->set_enabled(true);
            m_test_page_btn->invalidate();
        }
    }

} // namespace horizon::preferences
