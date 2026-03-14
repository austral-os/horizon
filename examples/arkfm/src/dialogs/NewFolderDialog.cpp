#include "dialogs/NewFolderDialog.hpp"
#include "horizon/AquaObject.hpp"
#include "horizon/Button.hpp"
#include "horizon/Label.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/TextBox.hpp"
#include "horizon/Window.hpp"

namespace horizon::arkfm
{
    NewFolderDialog::NewFolderDialog() : WaylandWindow("horizon.arkfm.new_folder", 400, 210)
    {
        set_name("Nueva Carpeta");
        setup_ui();
    }

    void NewFolderDialog::setup_ui()
    {
        auto window_widget = std::make_unique<horizon::Window>("Nueva Carpeta");

        auto root_panel = std::make_unique<horizon::Widget>();
        root_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root_panel->set_spacing(15);
        root_panel->set_margin(20);

        auto prompt_label = std::make_unique<horizon::Label>("Nombre de la carpeta:");
        prompt_label->set_font_size(14);
        prompt_label->set_fixed_size(25);
        root_panel->add_child(std::move(prompt_label));

        auto text_box = std::make_unique<horizon::TextBox<>>();
        text_box->set_placeholder("Nueva Carpeta");
        text_box->set_fixed_size(35);
        text_box->set_focusable(true);
        auto *text_box_ptr = text_box.get();
        root_panel->add_child(std::move(text_box));

        auto button_container = std::make_unique<horizon::Widget>();
        button_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_fixed_size(40);
        button_container->set_spacing(10);

        auto spacer = std::make_unique<horizon::Widget>();
        spacer->set_position_type(FILL);
        button_container->add_child(std::move(spacer));

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
            [this, text_box_ptr](auto &)
            {
                NewFolderEvent ev;
                ev.sender = this;
                ev.folder_name = text_box_ptr->text().empty() ? text_box_ptr->placeholder()
                                                              : text_box_ptr->text();
                when_accepted.run(ev);
                this->on_close();
            });
        button_container->add_child(std::move(accept_btn));

        root_panel->add_child(std::move(Spacer()));
        root_panel->add_child(std::move(button_container));

        window_widget->add_child(std::move(root_panel));
        set_root(std::move(window_widget));
    }
} // namespace horizon::arkfm
