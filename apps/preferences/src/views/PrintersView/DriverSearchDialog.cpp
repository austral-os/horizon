#include "views/PrintersView/DriverSearchDialog.hpp"
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/Application.hpp>
#include <horizon/Color.hpp>
#include <horizon/I18n.hpp>
#include <thread>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <memory>

namespace horizon::preferences
{
    DriverSearchDialog::DriverSearchDialog(const std::string& printer_name)
        : m_printer_name(printer_name)
    {
        set_name("DriverSearchDialog");

        auto win = std::make_unique<Window>(i18n().tr("preferences.printers.drivers"));

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_margin(20);
        content->set_spacing(15);

        // Header
        auto header = std::make_unique<Label>();
        header->set_text(i18n().tr("preferences.printers.driver_installation"));
        header->set_font_size(24);
        header->set_font_weight(FONT_WEIGHT_BOLD);
        header->set_fixed_size(30);
        content->add_child(std::move(header));

        auto desc = std::make_unique<Label>();
        desc->set_text(i18n().tr("preferences.printers.searching_drivers_for") + printer_name);
        desc->set_fixed_size(40);
        content->add_child(std::move(desc));

        auto status = std::make_unique<Label>();
        status->set_text(i18n().tr("preferences.printers.querying_repos"));
        status->set_font_size(12);
        status->set_text_color(Color("#666666"));
        status->set_fixed_size(20);
        m_status_label = status.get();
        content->add_child(std::move(status));

        auto loading = std::make_unique<LoadingBar>();
        loading->set_fixed_size(4); // alto
        m_loading_bar = loading.get();
        content->add_child(std::move(loading));

        // Table View (oculto inicialmente)
        auto table = std::make_unique<TableView<DriverPackage>>();
        table->set_header_visible(false);
        table->set_row_height(50);
        table->set_border_color(Color("#dddddd"));
        
        TableColumn<DriverPackage> col;
        col.width = 460;
        col.cell_factory = [](const DriverPackage& data) {
            auto container = std::make_unique<Widget>();
            container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            container->set_margin(8);
            
            auto title = std::make_unique<Label>();
            title->set_text(data.name);
            title->set_font_weight(FONT_WEIGHT_BOLD);
            title->set_fixed_size(20);
            
            auto sub = std::make_unique<Label>();
            sub->set_text(data.description);
            sub->set_font_size(12);
            sub->set_text_color(Color("#666666"));
            sub->set_fixed_size(20);
            
            container->add_child(std::move(title));
            container->add_child(std::move(sub));
            return container;
        };
        table->add_column(std::move(col));
        table->set_visible(false);
        
        table->when_row_click.connect([this](auto& ctx) {
            m_selected_package = ctx.row_data;
            if (m_install_btn) m_install_btn->set_enabled(true);
        });
        
        m_table_view = table.get();
        content->add_child(std::move(table));

        content->add_child(Spacer());

        // Botones
        auto bottom_row = std::make_unique<Widget>();
        bottom_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bottom_row->set_spacing(10);
        bottom_row->set_fixed_size(33);
        bottom_row->add_child(Spacer());

        auto manual_btn = std::make_unique<Button<AquaObject>>();
        manual_btn->set_text(i18n().tr("preferences.printers.manual_ppd"));
        manual_btn->set_fixed_size(180);
        manual_btn->set_visible(false);
        manual_btn->when_click.connect([this](auto&) {
            std::string ppd = "/tmp/manual.ppd"; 
            when_driver_ready.run(ppd);
            this->on_close();
        });
        m_manual_btn = manual_btn.get();
        bottom_row->add_child(std::move(manual_btn));

        auto cancel_btn = std::make_unique<Button<AquaObject>>();
        cancel_btn->set_text(i18n().tr("preferences.common.cancel"));
        cancel_btn->set_fixed_size(120);
        cancel_btn->when_click.connect([this](auto&) { this->on_close(); });
        m_cancel_btn = cancel_btn.get();
        bottom_row->add_child(std::move(cancel_btn));

        auto install_btn = std::make_unique<Button<AquaObject>>();
        install_btn->set_text(i18n().tr("preferences.printers.install"));
        install_btn->set_fixed_size(120);
        install_btn->set_enabled(false);
        install_btn->set_visible(false);
        install_btn->set_accent_color(WidgetAccentColor::Primary);
        install_btn->when_click.connect([this](auto&) { install_selected(); });
        m_install_btn = install_btn.get();
        bottom_row->add_child(std::move(install_btn));

        content->add_child(std::move(bottom_row));
        win->add_child(std::move(content));
        set_root(std::move(win));
        
        start_search();
    }

    DriverSearchDialog::~DriverSearchDialog() {}

    void DriverSearchDialog::start_search() {
        std::thread([this]() {
            // Extraer la marca (primera palabra de m_printer_name)
            std::string lowerName = m_printer_name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::string manufacturer = "printer";
            std::stringstream ss(lowerName);
            if (ss >> manufacturer) {
                if (manufacturer == "brother" || manufacturer == "hp" || manufacturer == "epson" || manufacturer == "canon" || manufacturer == "samsung") {
                    // valid
                } else {
                    manufacturer = "printer";
                }
            }

            std::string cmd = "apt-cache search " + manufacturer + " | grep -i printer";
            FILE* pipe = popen(cmd.c_str(), "r");
            
            std::vector<DriverPackage> pkgs;
            if (pipe) {
                char buffer[1024];
                while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                    std::string line(buffer);
                    if (line.back() == '\n') line.pop_back();
                    
                    size_t dashPos = line.find(" - ");
                    if (dashPos != std::string::npos) {
                        DriverPackage p;
                        p.name = line.substr(0, dashPos);
                        p.description = line.substr(dashPos + 3);
                        
                        // Filtrar solo los paquetes que parezcan drivers
                        if (p.name.find("printer-driver") != std::string::npos || p.name.find("hplip") != std::string::npos || p.name.find("cups") != std::string::npos) {
                            pkgs.push_back(p);
                        }
                    }
                }
                pclose(pipe);
            }

            this->post_task([this, pkgs]() {
                m_packages = pkgs;
                if (m_loading_bar) m_loading_bar->set_visible(false);
                
                if (m_packages.empty()) {
                    if (m_status_label) m_status_label->set_text(i18n().tr("preferences.printers.no_drivers_found"));
                    if (m_manual_btn) m_manual_btn->set_visible(true);
                } else {
                    if (m_status_label) m_status_label->set_text(i18n().tr("preferences.printers.select_driver"));
                    if (m_table_view) {
                        m_table_view->set_data(m_packages);
                        m_table_view->set_visible(true);
                    }
                    if (m_install_btn) m_install_btn->set_visible(true);
                    if (m_manual_btn) m_manual_btn->set_visible(true);
                }
                this->invalidate();
            });
            
        }).detach();
    }

    void DriverSearchDialog::install_selected() {
        if (!m_selected_package.has_value()) return;
        
        m_install_btn->set_enabled(false);
        m_install_btn->set_text(i18n().tr("preferences.printers.installing"));
        m_status_label->set_text(i18n().tr("preferences.printers.installing_package") + m_selected_package->name);
        m_loading_bar->set_visible(true);
        m_table_view->set_enabled(false);
        
        auto pkg = m_selected_package->name;
        
        std::thread([this, pkg]() {
            std::string cmd = "pkexec bash -c \"DEBIAN_FRONTEND=noninteractive apt-get install -y " + pkg + "\"";
            int ret = system(cmd.c_str());
            
            this->post_task([this, ret]() {
                if (ret == 0) {
                    std::string empty_str = "";
                    when_driver_ready.run(empty_str);
                    this->on_close();
                } else {
                    m_install_btn->set_text(i18n().tr("preferences.printers.failed"));
                    m_status_label->set_text(i18n().tr("preferences.printers.install_error"));
                    m_loading_bar->set_visible(false);
                }
            });
        }).detach();
    }
}
