#include "AuthDialog.hpp"
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Window.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace horizon::polkit
{
    AuthDialog::AuthDialog(const std::string &action_id, const std::string &message,
                           const std::string &user)
        : WaylandWindow("horizon.polkit.auth", 500, 400, true, true)
    {
        set_name(i18n().tr("core.polkit.auth_required"));
        setup_ui(message, user);
        set_min_size(450, 380);
    }

    void AuthDialog::setup_ui(const std::string &message, const std::string &user)
    {
        auto root_wnd = std::make_unique<Window>(name());
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(20);
        container->set_margin(20);

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name("dialog-password");
        icon->set_icon_size(64);
        icon->set_fixed_size(64);
        icon->set_vertical_alignment(VerticalAlignment::Top);

        auto text_container = std::make_unique<Widget>();
        text_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        text_container->set_spacing(8);
        text_container->set_fixed_size(40);

        auto title_label = std::make_unique<Label>(i18n().tr("core.polkit.auth_title"));
        title_label->set_font_weight(FONT_WEIGHT_BOLD);
        title_label->set_alignment(TextAlignment::Center);
        title_label->set_fixed_size(35);

        auto msg_label = std::make_unique<Label>(message);
        msg_label->set_alignment(TextAlignment::Center);

        text_container->add_child(std::move(title_label));
        text_container->add_child(std::move(msg_label));

        auto user_label =
            std::make_unique<Label>(i18n().tr("core.polkit.password_for") + ": " + user);
        user_label->set_alignment(TextAlignment::Left);
        user_label->set_margin(0);
        user_label->set_fixed_size(35);

        auto password_box = std::make_unique<TextBox<PasswordPolicy>>();
        password_box->set_placeholder(i18n().tr("core.polkit.enter_password"));
        password_box->set_margin(0);
        password_box->set_focusable(true);
        m_password_entry = password_box.get();

        m_password_entry->when_submit.connect(
            [this](KeyEventContext &ev)
            {
                on_authenticate();
            });

        // Label de error (inicialmente vacío y oculto)
        auto error_label = std::make_unique<Label>("");
        error_label->set_text_color(Color(1.0f, 0.2f, 0.2f, 1.0f)); // Rojo
        error_label->set_alignment(TextAlignment::Center);
        m_error_label = error_label.get();

        auto buttons_container = std::make_unique<Widget>();
        buttons_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons_container->set_fixed_size(35);

        auto cancel_btn = std::make_unique<Button<AquaObject>>();
        cancel_btn->set_text(i18n().tr("core.dialog.cancel"));
        cancel_btn->when_click.connect([this](MouseButtonEventContext &) { quit(); });

        auto auth_btn = std::make_unique<Button<AquaObject>>();
        auth_btn->set_text(i18n().tr("core.polkit.authenticate"));
        auth_btn->set_accent_color(WidgetAccentColor::Primary);
        auth_btn->when_click.connect([this](MouseButtonEventContext &) { on_authenticate(); });

        buttons_container->add_child(Spacer());
        buttons_container->add_child(std::move(cancel_btn));
        buttons_container->add_child(Spacer(20));
        buttons_container->add_child(std::move(auth_btn));

        container->add_child(std::move(icon));
        container->add_child(std::move(text_container));
        container->add_child(std::move(user_label));
        container->add_child(std::move(password_box));
        container->add_child(std::move(error_label));
        container->add_child(std::move(buttons_container));

        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));

        m_password_entry->set_focus(true);
    }

    void AuthDialog::on_authenticate()
    {
        // Limpiamos error anterior al reintentar
        m_error_label->set_text("");

        AuthSuccessEvent ev;
        ev.password = m_password_entry->text();
        when_authenticated.run(ev);

        // NO cerramos aquí. Esperamos respuesta de main.cpp
    }

    void AuthDialog::show_error(const std::string &error)
    {
        m_error_label->set_text(error);
        reset_password();
    }

    void AuthDialog::reset_password()
    {
        m_password_entry->set_text("");
        m_password_entry->set_focus(true);
    }
} // namespace horizon::polkit
