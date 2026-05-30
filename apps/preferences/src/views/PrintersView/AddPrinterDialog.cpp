#include <views/PrintersView/AddPrinterDialog.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Window.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <algorithm>
#include <iostream>

namespace horizon::preferences
{
    AddPrinterDialog::AddPrinterDialog()
        : WaylandWindow("horizon.add_printer", 550, 500, true, false)
    {
        set_use_global_menu(false);
        set_name(i18n().tr("preferences.printers.add_printer"));

        m_discovery = std::make_unique<horizon::print::PrinterDiscovery>();
        
        m_discovery->when_printer_found = [this](const horizon::print::Printer& p) {
            this->post_task([this, p]() {
                bool is_new = false;
                {
                    std::lock_guard<std::mutex> lock(m_printers_mutex);
                    bool exists = false;
                    for (const auto& existing : m_discovered_printers) {
                        if (existing.id == p.id) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        m_discovered_printers.push_back(p);
                        is_new = true;
                    }
                }
                if (is_new) {
                    this->filter_printers(m_search_box ? m_search_box->text() : "");
                }
            });
        };

        m_discovery->when_printer_lost = [this](const horizon::print::PrinterId& id) {
            this->post_task([this, id]() {
                bool removed = false;
                {
                    std::lock_guard<std::mutex> lock(m_printers_mutex);
                    auto it = std::remove_if(m_discovered_printers.begin(), m_discovered_printers.end(),
                        [&id](const horizon::print::Printer& p) { return p.id == id; });
                    if (it != m_discovered_printers.end()) {
                        m_discovered_printers.erase(it, m_discovered_printers.end());
                        removed = true;
                    }
                }
                if (removed) {
                    this->filter_printers(m_search_box ? m_search_box->text() : "");
                }
            });
        };

        setup_ui();
        start_scanning();
    }

    AddPrinterDialog::~AddPrinterDialog()
    {
        stop_scanning();
    }

    void AddPrinterDialog::setup_ui()
    {
        auto root_wnd = std::make_unique<Window>(i18n().tr("preferences.printers.add_printer"));
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto container = std::make_unique<Widget>();
        container->set_margin(20);
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(15);

        // 1. Title
        auto title = std::make_unique<Label>(i18n().tr("preferences.printers.add_printer"));
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_font_size(18);
        title->set_fixed_size(35);
        container->add_child(std::move(title));

        // 2. Search Box
        auto search_box = std::make_unique<TextBox<TextPolicy>>();
        search_box->set_placeholder(i18n().tr("preferences.common.search"));
        search_box->set_fixed_size(35);
        search_box->when_text_changed.connect([this](KeyEventContext &)
                                              { this->filter_printers(m_search_box->text()); });
        m_search_box = search_box.get();
        container->add_child(std::move(search_box));

        // 3. Table View
        auto table = std::make_unique<TableView<horizon::print::Printer>>();
        table->set_header_visible(false);

        TableColumn<horizon::print::Printer> icon_col;
        icon_col.width = 35;
        icon_col.cell_factory = [](const horizon::print::Printer &)
        {
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name("printer");
            icon->set_icon_size(20);
            return icon;
        };
        table->add_column(std::move(icon_col));

        TableColumn<horizon::print::Printer> name_col;
        name_col.width = 415;
        name_col.cell_factory = [](const horizon::print::Printer &data)
        {
            auto lbl = std::make_unique<Label>(data.name.empty() ? data.id : data.name);
            return lbl;
        };
        table->add_column(std::move(name_col));

        m_table_view = table.get();
        m_table_view->when_row_click.connect([this](auto &ctx)
                                             { this->on_printer_selected(ctx.row_data); });
        container->add_child(std::move(table));

        // 4. Bottom Status Area (LoadingBar)
        auto bottom_status = std::make_unique<Widget>();
        bottom_status->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bottom_status->set_spacing(10);
        bottom_status->set_fixed_size(35);

        auto refresh_icon = std::make_unique<Icon>();
        refresh_icon->set_icon_name("view-refresh");
        refresh_icon->set_fixed_size(20);
        bottom_status->add_child(std::move(refresh_icon));

        auto scan_label = std::make_unique<Label>(i18n().tr("preferences.printers.scanning"));
        m_status_label = scan_label.get();
        bottom_status->add_child(std::move(scan_label));

        bottom_status->add_child(Spacer(20));

        container->add_child(std::move(bottom_status));

        // Loading Bar
        auto loading_bar = std::make_unique<LoadingBar>();
        loading_bar->set_fixed_size(25);
        m_loading_bar = loading_bar.get();
        container->add_child(std::move(loading_bar));

        // 5. Buttons
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(40);
        buttons->set_spacing(10);
        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text(i18n().tr("preferences.common.cancel"));
        btn_cancel->set_fixed_size(100);
        btn_cancel->when_click.connect([this](MouseButtonEventContext &) { this->quit(); });
        m_cancel_btn = btn_cancel.get();
        buttons->add_child(std::move(btn_cancel));

        auto btn_add = std::make_unique<Button<AquaObject>>();
        btn_add->set_text(i18n().tr("preferences.printers.add"));
        btn_add->set_fixed_size(100);
        btn_add->set_accent_color(WidgetAccentColor::Primary);
        btn_add->set_enabled(false);
        btn_add->when_click.connect([this](MouseButtonEventContext &)
                                     { this->on_add_clicked(); });
        m_add_btn = btn_add.get();
        buttons->add_child(std::move(btn_add));

        container->add_child(std::move(buttons));
        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));
    }

    void AddPrinterDialog::start_scanning()
    {
        if (m_discovery) {
            m_discovery->startScan();
        }

        if (m_loading_bar)
            m_loading_bar->set_visible(true);
    }

    void AddPrinterDialog::stop_scanning()
    {
        if (m_discovery) {
            m_discovery->stopScan();
        }

        if (m_loading_bar)
            m_loading_bar->set_visible(false);
    }

    void AddPrinterDialog::filter_printers(const std::string &query)
    {
        std::lock_guard<std::mutex> lock(m_printers_mutex);

        if (query.empty())
        {
            m_table_view->set_data(m_discovered_printers);
            return;
        }

        std::vector<horizon::print::Printer> filtered;
        for (const auto &d : m_discovered_printers)
        {
            std::string name = d.name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::string q = query;
            std::transform(q.begin(), q.end(), q.begin(), ::tolower);

            if (name.find(q) != std::string::npos)
            {
                filtered.push_back(d);
            }
        }
        m_table_view->set_data(filtered);
    }

    void AddPrinterDialog::on_printer_selected(const horizon::print::Printer &printer)
    {
        m_selected_printer = printer;
        if (m_add_btn)
            m_add_btn->set_enabled(true);
    }

    void AddPrinterDialog::on_add_clicked()
    {
        if (m_selected_printer.id.empty())
            return;

        if (m_add_btn)
            m_add_btn->set_enabled(false);

        when_accepted.run(m_selected_printer);
        this->quit();
    }
}
