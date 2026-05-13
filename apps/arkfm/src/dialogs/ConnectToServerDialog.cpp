#include "dialogs/ConnectToServerDialog.hpp"
#include <filesystem>
#include <fstream>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <horizon/TableView.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Window.hpp>
#include <nlohmann/json.hpp>

namespace horizon::arkfm
{
    ConnectToServerDialog::ConnectToServerDialog() : WaylandWindow("arkfm.dialog", 550, 400)
    {
        set_name("Conectar al servidor");

        auto window_widget = std::make_unique<horizon::Window>("Conectar al servidor");

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_spacing(10);
        content->set_margin(20);

        auto label = std::make_unique<Label>("Ingrese la dirección del servidor:");
        label->set_fixed_size(35);
        content->add_child(std::move(label));

        auto input_row = std::make_unique<Widget>();
        input_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        input_row->set_spacing(10);
        input_row->set_fixed_size(30);

        auto combo = std::make_unique<Combo>();
        combo->set_fixed_size(150);
        combo->add_item("smb://", "Windows (SMB)");
        combo->add_item("ftp://", "FTP");
        combo->add_item("sftp://", "SFTP");
        combo->add_item("dav://", "WebDAV");
        m_protocol_combo = combo.get();

        auto address = std::make_unique<TextBox<>>();
        address->set_placeholder("192.168.1.100/share");
        address->set_fixed_size(-1);
        m_address_input = address.get();

        input_row->add_child(std::move(combo));
        input_row->add_child(std::move(address));
        content->add_child(std::move(input_row));

        auto history_label = std::make_unique<Label>("Recursos recientes:");
        history_label->set_font_weight(FONT_WEIGHT_BOLD);
        history_label->set_fixed_size(35);
        content->add_child(std::move(history_label));

        // TableView for history
        auto history_table = std::make_unique<TableView<std::string>>();
        m_history_table = history_table.get();
        m_history_table->set_header_visible(false);
        m_history_table->set_row_height(32);

        TableColumn<std::string> col;
        col.id = "uri";
        col.title = "Servidor";
        col.width = 500;
        col.cell_factory = [](const std::string &uri)
        {
            auto container = std::make_unique<Widget>();
            container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            container->set_spacing(8);
            container->set_margin(5);

            std::string icon_name = "folder-remote";
            std::string display_name = uri;

            if (uri.find("smb://") == 0)
            {
                icon_name = "network-server"; // Typical for SMB
                display_name = uri.substr(6);
            }
            else if (uri.find("ftp://") == 0)
            {
                icon_name = "network-server";
                display_name = uri.substr(6);
            }
            else if (uri.find("sftp://") == 0)
            {
                icon_name = "network-server";
                display_name = uri.substr(7);
            }
            else if (uri.find("dav://") == 0)
            {
                icon_name = "folder-remote";
                display_name = uri.substr(6);
            }

            auto icon = std::make_unique<Icon>();
            icon->set_icon_name(icon_name);
            icon->set_icon_size(16);
            icon->set_fixed_size(16);

            auto lbl = std::make_unique<Label>(display_name);

            container->add_child(std::move(icon));
            container->add_child(std::move(lbl));
            return container;
        };
        m_history_table->add_column(col);

        m_history_table->when_row_click.connect(
            [this](auto &ctx)
            {
                std::string full_uri = ctx.row_data;
                // Split URI into protocol and address
                size_t pos = full_uri.find("://");
                if (pos != std::string::npos)
                {
                    std::string protocol = full_uri.substr(0, pos + 3);
                    std::string addr = full_uri.substr(pos + 3);

                    if (m_protocol_combo)
                        m_protocol_combo->set_selected_item_by_id(protocol);
                    if (m_address_input)
                        m_address_input->set_text(addr);
                }
            });

        m_history_table->when_row_dbl_click.connect([this](auto &) { handle_connect(); });

        content->add_child(std::move(history_table));

        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_spacing(10);
        buttons->set_fixed_size(35);
        buttons->add_child(horizon::Spacer());

        auto cancel = std::make_unique<Button<AquaObject>>();
        cancel->set_text("Cancelar");
        cancel->set_width(100);
        cancel->when_click.connect([this](auto &) { quit(); });

        auto connect = std::make_unique<Button<AquaObject>>();
        connect->set_text("Conectar");
        connect->set_accent_color(WidgetAccentColor::Primary);
        connect->set_width(100);
        connect->when_click.connect([this](auto &) { handle_connect(); });

        buttons->add_child(std::move(cancel));
        buttons->add_child(std::move(connect));
        content->add_child(std::move(buttons));

        window_widget->add_child(std::move(content));
        set_root(std::move(window_widget));

        load_history();
    }

    void ConnectToServerDialog::handle_connect()
    {
        if (!m_protocol_combo || !m_address_input)
            return;

        const auto *item = m_protocol_combo->selected_item();
        if (!item)
            return;

        std::string uri = item->id + m_address_input->text();
        if (m_address_input->text().empty())
            return;

        // Add to history if not exists
        bool exists = false;
        for (const auto &h : m_history)
        {
            if (h == uri)
            {
                exists = true;
                break;
            }
        }

        if (!exists)
        {
            m_history.insert(m_history.begin(), uri);
            if (m_history.size() > 10)
                m_history.pop_back();
            save_history();
        }

        ConnectToServerEvent ev;
        ev.uri = uri;
        when_accepted.run(ev);
        quit();
    }

    void ConnectToServerDialog::load_history()
    {
        std::string home = getenv("HOME") ? getenv("HOME") : "/tmp";
        std::string path = home + "/.config/arkfm/server_history.json";

        if (std::filesystem::exists(path))
        {
            try
            {
                std::ifstream f(path);
                nlohmann::json j;
                f >> j;
                if (j.is_array())
                {
                    m_history = j.get<std::vector<std::string>>();
                }
            }
            catch (...)
            {
                LOG_ERROR << "Failed to load server history";
            }
        }

        if (m_history_table)
        {
            m_history_table->set_data(m_history);
        }
    }

    void ConnectToServerDialog::save_history()
    {
        std::string home = getenv("HOME") ? getenv("HOME") : "/tmp";
        std::string dir = home + "/.config/arkfm";
        std::string path = dir + "/server_history.json";

        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }

        try
        {
            std::ofstream f(path);
            nlohmann::json j = m_history;
            f << j.dump(4);
        }
        catch (...)
        {
            LOG_ERROR << "Failed to save server history";
        }
    }
} // namespace horizon::arkfm
