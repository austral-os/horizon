#include "dialogs/PropertiesDialog.hpp"
#include "ArkfmFileProvider.hpp"
#include "horizon/AquaObject.hpp"
#include "horizon/Button.hpp"
#include "horizon/Checkbox.hpp"
#include "horizon/Combo.hpp"
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
        : WaylandWindow("horizon.arkfm.properties", 650, 500, false, true), m_file_info(file_info)
    {
        set_name("Propiedades - " + ArkfmFileProvider::get_display_name(m_file_info));
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
        icon->set_icon_name(ArkfmFileProvider::get_icon_name(m_file_info));
        header_box->add_child(std::move(icon));

        auto name_label = std::make_unique<horizon::Label>(ArkfmFileProvider::get_display_name(m_file_info));
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
            lbl->set_fixed_size(150);
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
        permissions_tab->set_spacing(15);
        permissions_tab->set_margin(30);

        auto section_title = std::make_unique<horizon::Label>("Permisos de acceso");
        section_title->set_font_weight(FONT_WEIGHT_BOLD);
        section_title->set_alignment(TextAlignment::Center);
        permissions_tab->add_child(std::move(section_title));

        auto add_permission_row = [&](const std::string &label, const std::string &selected_id)
        {
            auto row = std::make_unique<horizon::Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_spacing(15);
            row->set_fixed_size(40);

            auto lbl = std::make_unique<horizon::Label>(label + ":");
            lbl->set_fixed_size(150);
            lbl->set_alignment(TextAlignment::Right);
            row->add_child(std::move(lbl));

            auto combo = std::make_unique<horizon::Combo>();
            combo->add_item("none", "Ninguno");
            combo->add_item("read", "Solo puede ver");
            combo->add_item("write", "Puede ver y modificar");
            combo->set_selected_item_by_id(selected_id);
            row->add_child(std::move(combo));

            permissions_tab->add_child(std::move(row));
        };

        auto get_perm_id = [&](uint32_t p, uint32_t r, uint32_t w)
        {
            if ((p & r) && (p & w))
                return "write";
            if (p & r)
                return "read";
            return "none";
        };

        add_permission_row("Propietario", get_perm_id(m_file_info.permissions, S_IRUSR, S_IWUSR));
        add_permission_row("Grupo", get_perm_id(m_file_info.permissions, S_IRGRP, S_IWGRP));
        add_permission_row("Otros", get_perm_id(m_file_info.permissions, S_IROTH, S_IWOTH));

        auto exec_row = std::make_unique<horizon::Widget>();
        exec_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        exec_row->set_spacing(15);
        exec_row->set_fixed_size(40);

        auto exec_lbl = std::make_unique<horizon::Label>("Ejecutar:");
        exec_lbl->set_fixed_size(150);
        exec_lbl->set_alignment(TextAlignment::Right);
        exec_row->add_child(std::move(exec_lbl));

        auto exec_check = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        exec_check->set_text("Permitir la ejecución del archivo como programa");
        exec_check->set_checked(m_file_info.permissions & (S_IXUSR | S_IXGRP | S_IXOTH));
        exec_row->add_child(std::move(exec_check));

        permissions_tab->add_child(std::move(exec_row));

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
