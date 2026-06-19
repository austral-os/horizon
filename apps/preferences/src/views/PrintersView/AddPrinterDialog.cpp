#include <views/PrintersView/AddPrinterDialog.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Window.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <algorithm>
#include <iostream>
#include <regex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <cups/cups.h>

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
        scan_label->set_fixed_size(200);
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
        buttons->set_fixed_size(33);
        buttons->set_spacing(10);
        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text(i18n().tr("preferences.common.cancel"));
        btn_cancel->set_fixed_size(120);
        btn_cancel->when_click.connect([this](MouseButtonEventContext &) { this->quit(); });
        m_cancel_btn = btn_cancel.get();
        buttons->add_child(std::move(btn_cancel));

        auto btn_add = std::make_unique<Button<AquaObject>>();
        btn_add->set_text(i18n().tr("preferences.printers.add"));
        btn_add->set_fixed_size(120);
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

namespace {
    bool probe_port(const std::string& ip, int port) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;

        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        int res = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (res < 0) {
            if (errno == EINPROGRESS) {
                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(sock, &fdset);
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 500000; // 500ms timeout

                res = select(sock + 1, NULL, &fdset, NULL, &tv);
                if (res == 1) {
                    int so_error;
                    socklen_t len = sizeof(so_error);
                    getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                    if (so_error == 0) {
                        close(sock);
                        return true;
                    }
                }
            }
        } else {
            close(sock);
            return true;
        }
        close(sock);
        return false;
    }

    std::string get_printer_model_ipp(const std::string& ip) {
        http_t* http = httpConnect(ip.c_str(), 631);
        if (!http) return "";
        
        ipp_t* request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);
        std::string uri = "ipp://" + ip + "/ipp/print";
        ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", nullptr, uri.c_str());
        
        const char* requested_attrs[] = { "printer-make-and-model" };
        ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "requested-attributes", 1, nullptr, requested_attrs);
        
        ipp_t* response = cupsDoRequest(http, request, "/ipp/print");
        std::string model;
        if (response) {
            ipp_attribute_t* attr = ippFindAttribute(response, "printer-make-and-model", IPP_TAG_TEXT);
            if (attr) {
                const char* str = ippGetString(attr, 0, nullptr);
                if (str) model = str;
            }
            ippDelete(response);
        }
        httpClose(http);
        return model;
    }

    std::string get_printer_model_pjl(const std::string& ip) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return "";

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9100);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return "";
        }

        std::string req = "\033%-12345X@PJL INFO ID\r\n\033%-12345X";
        send(sock, req.c_str(), req.size(), 0);

        char buf[1024];
        int n = recv(sock, buf, sizeof(buf) - 1, 0);
        close(sock);

        if (n > 0) {
            buf[n] = '\0';
            std::string resp(buf);
            std::string mfg;
            std::string mdl;

            size_t pos = resp.find("MFG:");
            if (pos != std::string::npos) {
                size_t end = resp.find(";", pos);
                if (end != std::string::npos) mfg = resp.substr(pos + 4, end - pos - 4);
            }
            pos = resp.find("MDL:");
            if (pos != std::string::npos) {
                size_t end = resp.find(";", pos);
                if (end != std::string::npos) mdl = resp.substr(pos + 4, end - pos - 4);
            }

            if (!mfg.empty() && !mdl.empty()) {
                return mfg + " " + mdl;
            } else if (!mdl.empty()) {
                return mdl;
            }
        }
        return "";
    }

    std::string get_printer_model_http(const std::string& ip) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return "";

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(80);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return "";
        }

        std::string req = "GET / HTTP/1.0\r\nHost: " + ip + "\r\nConnection: close\r\n\r\n";
        send(sock, req.c_str(), req.size(), 0);

        char buf[4096];
        int n = recv(sock, buf, sizeof(buf) - 1, 0);
        close(sock);

        if (n > 0) {
            buf[n] = '\0';
            std::string resp(buf);
            
            size_t title_start = resp.find("<title>");
            if (title_start == std::string::npos) title_start = resp.find("<TITLE>");
            if (title_start != std::string::npos) {
                size_t title_end = resp.find("</title>", title_start);
                if (title_end == std::string::npos) title_end = resp.find("</TITLE>", title_start);
                if (title_end != std::string::npos) {
                    std::string title = resp.substr(title_start + 7, title_end - title_start - 7);
                    // Limpiar titulo
                    while(!title.empty() && (title.back() == '\r' || title.back() == '\n' || title.back() == ' ')) title.pop_back();
                    size_t first = title.find_first_not_of(" \r\n");
                    if (first != std::string::npos) title = title.substr(first);
                    return title;
                }
            }
        }
        return "";
    }
}

    void AddPrinterDialog::filter_printers(const std::string &query)
    {
        std::lock_guard<std::mutex> lock(m_printers_mutex);

        std::vector<horizon::print::Printer> all_printers = m_discovered_printers;
        all_printers.insert(all_printers.end(), m_manual_printers.begin(), m_manual_printers.end());

        std::vector<horizon::print::Printer> filtered;

        if (query.empty())
        {
            filtered = all_printers;
        }
        else
        {
            for (const auto &d : all_printers)
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

            std::regex ip_regex("([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})");
            std::smatch match;
            if (std::regex_search(query, match, ip_regex)) {
                std::string ip = match[1].str();
                std::string extra = query;
                extra.erase(extra.find(ip), ip.length());
                // trim spaces
                while(!extra.empty() && extra.back()==' ') extra.pop_back();
                while(!extra.empty() && extra.front()==' ') extra.erase(extra.begin());
                
                if (m_probed_ips.find(ip) == m_probed_ips.end() || !extra.empty()) {
                    m_probed_ips.insert(ip);
                    std::thread([this, ip, extra]() {
                        std::string name = extra;
                        std::string uri = "";
                        
                        if (probe_port(ip, 9100)) {
                            if (name.empty()) {
                                std::string pjl_name = get_printer_model_pjl(ip);
                                if (!pjl_name.empty()) name = pjl_name;
                            }
                            uri = "socket://" + ip + ":9100";
                        }
                        
                        if (uri.empty() && probe_port(ip, 631)) {
                            if (name.empty()) {
                                std::string ipp_name = get_printer_model_ipp(ip);
                                if (!ipp_name.empty()) name = ipp_name;
                            }
                            uri = "ipp://" + ip + "/ipp/print";
                        }
                        
                        if (uri.empty() && probe_port(ip, 515)) {
                            uri = "lpd://" + ip + "/queue";
                        }
                        
                        // Si se detecto el puerto pero aun no hay nombre, intentar HTTP scraper
                        if (!uri.empty() && name.empty()) {
                            std::string http_name = get_printer_model_http(ip);
                            if (!http_name.empty()) name = http_name;
                        }
                        
                        if (!uri.empty()) {
                            if (name.empty()) name = "Unknown Network Device (" + ip + ")";
                            
                            this->post_task([this, ip, name, uri]() {
                                {
                                    std::lock_guard<std::mutex> lk(m_printers_mutex);
                                    // Remove any existing manual printer with same IP to update name
                                    m_manual_printers.erase(
                                        std::remove_if(m_manual_printers.begin(), m_manual_printers.end(),
                                            [&](const horizon::print::Printer& p) { return p.id == "ip_" + ip; }),
                                        m_manual_printers.end()
                                    );
                                    
                                    horizon::print::Printer ip_printer;
                                    ip_printer.id = "ip_" + ip;
                                    ip_printer.name = name;
                                    ip_printer.uri = uri;
                                    ip_printer.source = horizon::print::PrinterSource::Discovered;
                                    m_manual_printers.push_back(ip_printer);
                                }
                                if (m_search_box) {
                                    this->filter_printers(m_search_box->text());
                                }
                            });
                        }
                    }).detach();
                }
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
