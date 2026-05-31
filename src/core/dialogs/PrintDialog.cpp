#include "horizon/Spacer.hpp"
#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/dialogs/PrintDialog.hpp>
#include <horizon/Label.hpp>
#include <horizon/Combo.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/I18n.hpp>
#include <iostream>
#include <thread>

namespace horizon
{

    PrintDialog::PrintDialog(Widget *print_target) : m_print_target(print_target)
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
        auto win = std::make_unique<Window>(i18n().tr("core.print.title"));
        win->set_size(600, 600); // Increased size to fit new controls

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_position_type(FILL);
        content->set_margin(15);
        content->set_spacing(15);

        auto header = std::make_unique<Label>();
        header->set_text(i18n().tr("core.print.title"));
        header->set_font_size(24);
        header->set_font_weight(FONT_WEIGHT_BOLD);
        header->set_fixed_size(30);
        content->add_child(std::move(header));

        auto desc = std::make_unique<Label>();
        desc->set_text(i18n().tr("core.print.select_printer"));
        desc->set_fixed_size(20);
        content->add_child(std::move(desc));

        auto table = std::make_unique<TableView<horizon::print::Printer>>();
        table->set_position_type(FILL);

        TableColumn<horizon::print::Printer> col_name;
        col_name.id = "name";
        col_name.title = i18n().tr("core.print.name");
        col_name.width = 150;
        col_name.cell_factory = [](const horizon::print::Printer &p)
        {
            auto lbl = std::make_unique<Label>();
            lbl->set_text(p.name);
            return lbl;
        };
        table->add_column(std::move(col_name));

        TableColumn<horizon::print::Printer> col_uri;
        col_uri.id = "uri";
        col_uri.title = "URI";
        col_uri.width = 250;
        col_uri.cell_factory = [](const horizon::print::Printer &p)
        {
            auto lbl = std::make_unique<Label>();
            lbl->set_text(p.uri);
            return lbl;
        };
        table->add_column(std::move(col_uri));

        m_printer_list = table.get();

        table->when_row_click.connect(
            [this](const TableViewRowMouseClickContext<horizon::print::Printer> &ctx)
            {
                int index = ctx.row_index;
                if (index >= 0 && index < (int)m_printers.size())
                {
                    m_selected_printer = m_printers[index];
                    m_print_btn->set_enabled(true);
                }
                else
                {
                    m_selected_printer = std::nullopt;
                    m_print_btn->set_enabled(false);
                }
            });

        content->add_child(std::move(table));

        // Advanced settings container
        auto settings_row1 = std::make_unique<Widget>();
        settings_row1->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        settings_row1->set_spacing(15);
        settings_row1->set_fixed_size(30);

        auto paper_lbl = std::make_unique<Label>();
        paper_lbl->set_text("Papel:");
        paper_lbl->set_fixed_size(80);
        settings_row1->add_child(std::move(paper_lbl));

        auto paper_combo = std::make_unique<Combo>();
        paper_combo->add_item("A4", "A4");
        paper_combo->add_item("Letter", i18n().tr("core.print.paper_letter"));
        paper_combo->add_item("Legal", i18n().tr("core.print.paper_legal"));
        paper_combo->add_item("Executive", i18n().tr("core.print.paper_exec"));
        paper_combo->set_selected_item_index(0);
        m_paper_size_combo = paper_combo.get();
        settings_row1->add_child(std::move(paper_combo));

        auto orient_lbl = std::make_unique<Label>();
        orient_lbl->set_text(i18n().tr("core.print.orientation"));
        orient_lbl->set_alignment(TextAlignment::Right);
        settings_row1->add_child(std::move(orient_lbl));

        auto orient_combo = std::make_unique<Combo>();
        orient_combo->add_item("Portrait", i18n().tr("core.print.portrait"));
        orient_combo->add_item("Landscape", i18n().tr("core.print.landscape"));
        orient_combo->set_selected_item_index(0);
        m_orientation_combo = orient_combo.get();
        settings_row1->add_child(std::move(orient_combo));

        content->add_child(std::move(settings_row1));

        auto settings_row2 = std::make_unique<Widget>();
        settings_row2->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        settings_row2->set_spacing(15);
        settings_row2->set_fixed_size(30);

        auto quality_lbl = std::make_unique<Label>();
        quality_lbl->set_text("Calidad:");
        quality_lbl->set_fixed_size(80);
        settings_row2->add_child(std::move(quality_lbl));

        auto quality_combo = std::make_unique<Combo>();
        quality_combo->add_item("Normal", i18n().tr("core.print.normal"));
        quality_combo->add_item("Draft", i18n().tr("core.print.draft"));
        quality_combo->add_item("High", i18n().tr("core.print.high"));
        quality_combo->set_selected_item_index(0);
        m_quality_combo = quality_combo.get();
        settings_row2->add_child(std::move(quality_combo));

        auto pages_lbl = std::make_unique<Label>();
        pages_lbl->set_text(i18n().tr("core.print.pages"));
        pages_lbl->set_alignment(TextAlignment::Right);
        settings_row2->add_child(std::move(pages_lbl));

        auto pages_combo = std::make_unique<Combo>();
        pages_combo->add_item("all", i18n().tr("core.print.all"));
        pages_combo->add_item("even", i18n().tr("core.print.even"));
        pages_combo->add_item("odd", i18n().tr("core.print.odd"));
        pages_combo->add_item("range", i18n().tr("core.print.specific"));
        pages_combo->set_selected_item_index(0);
        m_page_range_combo = pages_combo.get();
        settings_row2->add_child(std::move(pages_combo));

        content->add_child(std::move(settings_row2));

        auto settings_row3 = std::make_unique<Widget>();
        settings_row3->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        settings_row3->set_spacing(15);
        settings_row3->set_fixed_size(30);

        settings_row3->add_child(std::move(Spacer()));

        auto range_text = std::make_unique<TextBox<>>();
        range_text->set_placeholder(i18n().tr("core.print.range_placeholder"));
        range_text->set_fixed_size(120);
        range_text->set_enabled(false);
        range_text->set_visible(false);
        m_page_range_text = range_text.get();
        settings_row3->add_child(std::move(range_text));

        m_page_range_combo->when_item_selected.connect(
            [this](const ComboItemSelectedContext &ctx)
            {
                if (ctx.item.id == "range")
                {
                    m_page_range_text->set_enabled(true);
                    m_page_range_text->set_visible(true);
                }
                else
                {
                    m_page_range_text->set_enabled(false);
                    m_page_range_text->set_visible(false);
                }
            });

        content->add_child(std::move(settings_row3));

        auto btn_row = std::make_unique<Widget>();
        btn_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_row->set_fixed_size(33);
        btn_row->set_spacing(10);

        auto spacer = std::make_unique<Widget>();
        spacer->set_position_type(FILL);
        btn_row->add_child(std::move(spacer));

        auto cancel_btn = std::make_unique<Button<AquaObject>>();
        cancel_btn->set_text(i18n().tr("core.dialog.cancel"));
        cancel_btn->set_fixed_size(120);
        m_cancel_btn = cancel_btn.get();
        m_cancel_btn->when_click.connect([this](auto &) { this->quit(); });
        btn_row->add_child(std::move(cancel_btn));

        auto print_btn = std::make_unique<Button<AquaObject>>();
        print_btn->set_text(i18n().tr("core.print.print_btn"));
        print_btn->set_fixed_size(120);
        print_btn->set_enabled(false);
        print_btn->set_accent_color(WidgetAccentColor::Primary);
        m_print_btn = print_btn.get();
        m_print_btn->when_click.connect([this](auto &) { this->print(); });
        btn_row->add_child(std::move(print_btn));

        content->add_child(std::move(btn_row));

        win->add_child(std::move(content));
        set_root(std::move(win));
    }

    void PrintDialog::load_printers()
    {
        m_printers.clear();

        std::thread(
            [this]()
            {
                auto printers = m_backend->listPrinters();

                this->post_task(
                    [this, printers]()
                    {
                        m_printers = printers;
                        m_printer_list->set_data(m_printers);
                    });
            })
            .detach();
    }

    void PrintDialog::print()
    {
        if (!m_selected_printer || !m_print_target)
            return;

        m_print_btn->set_enabled(false);
        m_print_btn->set_text("Enviando...");

        horizon::print::PrintConfig config;

        // Populating config from UI
        if (m_paper_size_combo && m_paper_size_combo->selected_item())
        {
            config.paper_size = m_paper_size_combo->selected_item()->id;
        }

        if (m_orientation_combo && m_orientation_combo->selected_item())
        {
            std::string orient = m_orientation_combo->selected_item()->id;
            config.orientation = (orient == "Landscape") ? horizon::print::Orientation::Landscape
                                                         : horizon::print::Orientation::Portrait;
        }

        if (m_quality_combo && m_quality_combo->selected_item())
        {
            std::string q = m_quality_combo->selected_item()->id;
            if (q == "Draft")
                config.quality = horizon::print::PrintQuality::Draft;
            else if (q == "High")
                config.quality = horizon::print::PrintQuality::High;
            else
                config.quality = horizon::print::PrintQuality::Normal;
        }

        if (m_page_range_combo && m_page_range_combo->selected_item())
        {
            std::string pr = m_page_range_combo->selected_item()->id;
            if (pr == "range" && m_page_range_text)
            {
                config.page_ranges = m_page_range_text->text();
            }
            else if (pr != "range")
            {
                config.page_set = pr; // "all", "even", "odd"
            }
        }

        auto target = m_print_target;
        std::string printer_id = m_selected_printer->id;
        auto backend = m_backend;

        std::thread(
            [this, target, printer_id, backend, config]()
            {
                auto doc = target->generate_print_document(config);

                if (doc.isValid())
                {
                    auto job_id = backend->submitJob(printer_id, doc, config);
                    if (!job_id.empty())
                    {
                        std::cout << "Print job submitted successfully: " << job_id << std::endl;
                        this->post_task([this]() { this->quit(); });
                        return;
                    }
                }

                this->post_task(
                    [this]()
                    {
                        m_print_btn->set_text("Error");
                        m_print_btn->set_enabled(true);
                    });
            })
            .detach();
    }

} // namespace horizon
