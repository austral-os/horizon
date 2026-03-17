#include "dialogs/PropertiesDialog.hpp"
#include "ArkfmIconProvider.hpp"
#include "horizon/AquaObject.hpp"
#include "horizon/Button.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Label.hpp"
#include "horizon/Notebook.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/Window.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace horizon::arkfm
{
    PropertiesDialog::PropertiesDialog(const arkutils::FileInfo &file_info)
        : WaylandWindow("horizon.arkfm.properties", 650, 500, false, false), m_file_info(file_info)
    {
        set_name("Propiedades - " + m_file_info.name);
        setup_ui();
    }

    void PropertiesDialog::setup_ui()
    {
        auto window_widget = std::make_unique<horizon::Window>("Propiedades");

        auto root_panel = std::make_unique<horizon::Widget>();
        root_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root_panel->set_spacing(10);
        root_panel->set_margin(15);

        auto notebook = std::make_unique<horizon::Notebook>();

        // --- General Tab ---
        auto general_tab = std::make_unique<horizon::Widget>();
        general_tab->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        general_tab->set_spacing(15);
        general_tab->set_margin(20);

        auto header_box = std::make_unique<horizon::Widget>();
        header_box->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header_box->set_spacing(15);
        header_box->set_fixed_size(64);

        auto icon = std::make_unique<horizon::Icon>();
        icon->set_icon_size(64);
        icon->set_icon_name(ArkfmIconProvider::get_icon_name(m_file_info));
        header_box->add_child(std::move(icon));

        auto name_label = std::make_unique<horizon::Label>(m_file_info.name);
        name_label->set_font_weight(FONT_WEIGHT_BOLD);
        header_box->add_child(std::move(name_label));

        general_tab->add_child(std::move(header_box));

        auto info_grid = std::make_unique<horizon::Widget>();
        info_grid->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        info_grid->set_spacing(8);

        auto add_info_row = [&](const std::string &label, const std::string &value)
        {
            auto row = std::make_unique<horizon::Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_spacing(10);
            row->set_fixed_size(24);

            auto lbl = std::make_unique<horizon::Label>(label + ":");
            lbl->set_fixed_size(100);
            row->add_child(std::move(lbl));

            auto val = std::make_unique<horizon::Label>(value);
            row->add_child(std::move(val));

            info_grid->add_child(std::move(row));
        };

        std::string type_str =
            (m_file_info.type == arkutils::FileType::Directory) ? "Carpeta" : "Archivo";
        add_info_row("Tipo", type_str);
        add_info_row("Tamaño", std::to_string(m_file_info.size / 1024) + " KB");
        add_info_row("Ubicación", m_file_info.path);

        auto t = std::chrono::system_clock::to_time_t(m_file_info.last_modified);
        char time_str[26] = {0};
        ctime_r(&t, time_str);
        if (time_str[0] != '\0')
        {
            time_str[24] = '\0'; // Remove newline
        }
        add_info_row("Modificado", std::string(time_str));

        general_tab->add_child(std::move(info_grid));
        notebook->add_tab(NotebookPage("General", std::move(general_tab)));

        // --- Permissions Tab ---
        auto permissions_tab = std::make_unique<horizon::Widget>();
        permissions_tab->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        permissions_tab->set_spacing(10);
        permissions_tab->set_margin(20);
        permissions_tab->add_child(std::make_unique<horizon::Label>("Aquí van los permisos."));
        notebook->add_tab(NotebookPage("Permisos", std::move(permissions_tab)));

        // --- Details Tab ---
        auto details_tab = std::make_unique<horizon::Widget>();
        details_tab->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        details_tab->set_spacing(10);
        details_tab->set_margin(20);
        details_tab->add_child(std::make_unique<horizon::Label>("Aquí detalles."));
        notebook->add_tab(NotebookPage("Detalles", std::move(details_tab)));

        root_panel->add_child(std::move(notebook));

        // --- Button Container ---
        auto button_container = std::make_unique<horizon::Widget>();
        button_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_fixed_size(40);
        button_container->set_spacing(10);

        button_container->add_child(std::move(Spacer()));

        auto cancel_btn = std::make_unique<horizon::Button<horizon::AquaObject>>();
        cancel_btn->set_text("Cancelar");
        cancel_btn->set_size(100, 35);
        cancel_btn->when_click.connect(
            [this](auto &)
            {
                EventContext ev;
                ev.sender = this;
                when_cancelled.run(ev);
                this->on_close();
            });
        button_container->add_child(std::move(cancel_btn));

        auto accept_btn = std::make_unique<horizon::Button<horizon::AquaObject>>();
        accept_btn->set_text("Aceptar");
        accept_btn->set_size(100, 35);
        accept_btn->set_accent_color(WidgetAccentColor::Primary);
        accept_btn->when_click.connect(
            [this](auto &)
            {
                EventContext ev;
                ev.sender = this;
                when_accepted.run(ev);
                this->on_close();
            });
        button_container->add_child(std::move(accept_btn));

        root_panel->add_child(std::move(button_container));

        window_widget->add_child(std::move(root_panel));
        set_root(std::move(window_widget));
    }
} // namespace horizon::arkfm
