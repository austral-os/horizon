#include "views/PrintersView/DriverSearchDialog.hpp"
#include <algorithm>
#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/Color.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/TextBox.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

namespace horizon::preferences
{
    DriverSearchDialog::DriverSearchDialog(const std::string &printer_name)
        : m_printer_name(printer_name)
    {
        set_name("DriverSearchDialog");
        setup_ui();
    }

    DriverSearchDialog::~DriverSearchDialog() = default;

    void DriverSearchDialog::setup_ui()
    {
        auto win = std::make_unique<Window>(i18n().tr("preferences.printers.search_driver"));
        win->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_position_type(WidgetPositionTypes::FILL);
        content->set_margin(20);
        content->set_spacing(15);

        auto title = std::make_unique<Label>();
        title->set_text(i18n().tr("preferences.printers.driver_for") + " " + m_printer_name);
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_font_size(16);
        title->set_fixed_size(35);
        content->add_child(std::move(title));

        auto status = std::make_unique<Label>();
        status->set_text(i18n().tr("preferences.printers.searching_drivers"));
        status->set_text_color(Color("#666666"));
        status->set_fixed_size(35);
        m_status_label = status.get();
        content->add_child(std::move(status));

        auto search_box = std::make_unique<TextBox<>>();
        search_box->set_placeholder("Search driver...");
        search_box->set_fixed_size(30);
        search_box->when_text_changed.connect(
            [this](const KeyEventContext &)
            {
                if (this->m_search_box)
                {
                    this->filter_drivers(this->m_search_box->text());
                }
            });
        m_search_box = search_box.get();
        content->add_child(std::move(search_box));

        auto bar = std::make_unique<LoadingBar>();
        bar->set_fixed_size(4);
        bar->set_position_type(WidgetPositionTypes::FILL);
        m_loading_bar = bar.get();
        content->add_child(std::move(bar));

        auto table = std::make_unique<TableView<DriverPackage>>();
        table->set_position_type(WidgetPositionTypes::FILL);
        table->set_header_visible(false);
        table->set_row_height(50);
        table->set_border_color(Color("#dddddd"));

        TableColumn<DriverPackage> col;
        col.width = 460;
        col.cell_factory = [](const DriverPackage &data)
        {
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

        table->when_row_click.connect(
            [this](auto &ctx)
            {
                m_selected_package = ctx.row_data;
                if (m_install_btn)
                    m_install_btn->set_enabled(true);
            });

        m_table_view = table.get();
        content->add_child(std::move(table));

        auto bottom_row = std::make_unique<Widget>();
        bottom_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bottom_row->set_spacing(10);
        bottom_row->set_fixed_size(33);
        bottom_row->add_child(Spacer());

        auto manual_btn = std::make_unique<Button<AquaObject>>();
        manual_btn->set_text(i18n().tr("preferences.printers.manual_ppd"));
        manual_btn->set_fixed_size(180);
        manual_btn->set_visible(false);
        manual_btn->when_click.connect(
            [this](auto &)
            {
                DriverPackage pkg;
                pkg.name = "/tmp/manual.ppd";
                pkg.description = "Manual Printer";
                when_driver_ready.run(pkg);
                this->on_close();
            });
        m_manual_btn = manual_btn.get();
        bottom_row->add_child(std::move(manual_btn));

        auto cancel_btn = std::make_unique<Button<AquaObject>>();
        cancel_btn->set_text(i18n().tr("preferences.common.cancel"));
        cancel_btn->set_fixed_size(120);
        cancel_btn->when_click.connect([this](auto &) { this->on_close(); });
        m_cancel_btn = cancel_btn.get();
        bottom_row->add_child(std::move(cancel_btn));

        auto install_btn = std::make_unique<Button<AquaObject>>();
        install_btn->set_text("Select Driver");
        install_btn->set_fixed_size(120);
        install_btn->set_enabled(false);
        install_btn->set_accent_color(WidgetAccentColor::Primary);
        install_btn->when_click.connect([this](auto &) { install_selected(); });
        m_install_btn = install_btn.get();
        bottom_row->add_child(std::move(install_btn));

        content->add_child(std::move(bottom_row));
        win->add_child(std::move(content));
        set_root(std::move(win));

        start_search();
    }

    void DriverSearchDialog::filter_drivers(const std::string &query)
    {
        std::vector<DriverPackage> filtered;
        if (query.empty())
        {
            filtered = m_packages;
        }
        else
        {
            std::string q = query;
            std::transform(q.begin(), q.end(), q.begin(), ::tolower);
            for (const auto &p : m_packages)
            {
                std::string desc = p.description;
                std::string name = p.name;
                std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (desc.find(q) != std::string::npos || name.find(q) != std::string::npos)
                {
                    filtered.push_back(p);
                }
            }
        }
        if (m_table_view)
            m_table_view->set_data(filtered);
    }

    void DriverSearchDialog::start_search()
    {
        std::thread(
            [this]()
            {
                std::vector<DriverPackage> pkgs;
                FILE *pipe = popen("/usr/sbin/lpinfo -m", "r");
                if (pipe)
                {
                    char buffer[1024];
                    while (fgets(buffer, sizeof(buffer), pipe) != NULL)
                    {
                        std::string line(buffer);
                        if (line.back() == '\n')
                            line.pop_back();

                        size_t spacePos = line.find(' ');
                        if (spacePos != std::string::npos)
                        {
                            DriverPackage p;
                            p.name = line.substr(0, spacePos);         // PPD path
                            p.description = line.substr(spacePos + 1); // display name
                            pkgs.push_back(p);
                        }
                    }
                    pclose(pipe);
                }

                this->post_task(
                    [this, pkgs]()
                    {
                        m_packages = pkgs;
                        if (m_loading_bar)
                            m_loading_bar->set_visible(false);

                        if (m_status_label)
                            m_status_label->set_text("Select a driver from the list:");

                        // Extract brand for initial filter
                        std::string lowerName = m_printer_name;
                        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                                       ::tolower);
                        std::string manufacturer = "printer";
                        std::stringstream ss(lowerName);
                        if (ss >> manufacturer)
                        {
                            if (manufacturer != "generic" && manufacturer != "printer" &&
                                manufacturer != "unknown")
                            {
                                if (m_search_box)
                                {
                                    m_search_box->set_text(manufacturer);
                                    this->filter_drivers(manufacturer);
                                }
                                else
                                {
                                    this->filter_drivers("");
                                }
                            }
                            else
                            {
                                this->filter_drivers("");
                            }
                        }
                        else
                        {
                            this->filter_drivers("");
                        }

                        this->invalidate();
                    });
            })
            .detach();
    }

    void DriverSearchDialog::install_selected()
    {
        if (!m_selected_package.has_value())
            return;

        m_install_btn->set_enabled(false);
        m_install_btn->set_text(i18n().tr("preferences.printers.installing"));
        m_loading_bar->set_visible(true);
        m_table_view->set_enabled(false);

        // Pass the DriverPackage back immediately since it's already installed
        when_driver_ready.run(*m_selected_package);
        this->on_close();
    }
} // namespace horizon::preferences
