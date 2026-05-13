#include "ItemDialog.hpp"
#include <horizon/Spacer.hpp>
#include <horizon/Window.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/AquaObject.hpp>

namespace horizon::keyring
{
    ItemDialog::ItemDialog(const std::string& title) 
        : WaylandWindow("horizon.keyring.dialog", 400, 350, false, false)
    {
        set_name(title);
        setup_ui();
    }

    void ItemDialog::setup_ui()
    {
        auto window_widget = std::make_unique<horizon::Window>(name());

        auto root_panel = std::make_unique<horizon::Widget>();
        root_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root_panel->set_spacing(15);
        root_panel->set_margin(20);

        // Label
        root_panel->add_child(std::make_unique<Label>("Nombre:"));
        auto txt_label = std::make_unique<TextBox<TextPolicy>>();
        m_txt_label = txt_label.get();
        m_txt_label->set_fixed_size(35);
        root_panel->add_child(std::move(txt_label));

        // Secret
        root_panel->add_child(std::make_unique<Label>("Contraseña:"));
        auto txt_secret = std::make_unique<TextBox<PasswordPolicy>>();
        m_txt_secret = txt_secret.get();
        m_txt_secret->set_fixed_size(35);
        root_panel->add_child(std::move(txt_secret));

        // Type
        root_panel->add_child(std::make_unique<Label>("Tipo:"));
        auto cmb_type = std::make_unique<horizon::Combo>();
        m_cmb_type = cmb_type.get();
        m_cmb_type->add_item("Password", "Contraseña", "dialog-password-symbolic");
        m_cmb_type->add_item("Key", "Llave", "key-symbolic");
        m_cmb_type->add_item("Certificate", "Certificado", "certificate-symbolic");
        m_cmb_type->set_selected_item_index(0);
        m_cmb_type->set_fixed_size(35);
        root_panel->add_child(std::move(cmb_type));

        root_panel->add_child(Spacer());

        // Buttons
        auto button_container = std::make_unique<horizon::Widget>();
        button_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_fixed_size(40);
        button_container->set_spacing(10);
        
        button_container->add_child(Spacer());

        auto cancel_btn = std::make_unique<horizon::Button<horizon::AquaObject>>();
        cancel_btn->set_text("Cancelar");
        cancel_btn->set_size(100, 35);
        cancel_btn->when_click.connect([this](auto&) {
            EventContext ev;
            ev.sender = this;
            when_cancelled.run(ev);
            this->on_close();
        });
        button_container->add_child(std::move(cancel_btn));

        auto accept_btn = std::make_unique<horizon::Button<horizon::AquaObject>>();
        accept_btn->set_text("Guardar");
        accept_btn->set_size(100, 35);
        accept_btn->set_accent_color(WidgetAccentColor::Primary);
        accept_btn->when_click.connect([this](auto&) {
            ItemEvent ev;
            ev.sender = this;
            ev.label = m_txt_label->text();
            ev.secret = m_txt_secret->text();
            ev.type = m_cmb_type->selected_item() ? m_cmb_type->selected_item()->id : "Password";
            when_accepted.run(ev);
            this->on_close();
        });
        button_container->add_child(std::move(accept_btn));

        root_panel->add_child(std::move(button_container));

        window_widget->add_child(std::move(root_panel));
        set_root(std::move(window_widget));
    }

    void ItemDialog::set_initial_values(const std::string& label, const std::string& secret, const std::string& type)
    {
        if (m_txt_label) m_txt_label->set_text(label);
        if (m_txt_secret) m_txt_secret->set_text(secret);
        if (m_cmb_type) m_cmb_type->set_selected_item_by_id(type);
    }
}
