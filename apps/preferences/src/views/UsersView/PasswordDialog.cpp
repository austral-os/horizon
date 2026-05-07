#include <horizon/AquaObject.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Window.hpp>
#include <views/UsersView/PasswordDialog.hpp>

namespace horizon::preferences
{
    PasswordDialog::PasswordDialog(const std::string &username)
        : WaylandWindow("horizon.preferences.password-dialog", 400, 360, false),
          m_username(username)
    {
        set_name(i18n().tr("preferences.users.change_password"));
        setup_ui();
    }

    void PasswordDialog::setup_ui()
    {
        auto window_widget =
            std::make_unique<horizon::Window>(i18n().tr("preferences.users.change_password"));

        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root->set_margin(25);

        // Pass 1

        auto lbl1 = std::make_unique<Label>(i18n().tr("preferences.users.new_password") + ":");
        lbl1->set_fixed_size(45);
        root->add_child(std::move(lbl1));

        auto box1 = std::make_unique<TextBox<PasswordPolicy>>();
        m_pass1_box = box1.get();
        m_pass1_box->set_fixed_size(35);
        m_pass1_box->set_focusable(true);
        root->add_child(std::move(box1));

        root->add_child(Spacer(10));

        auto lbl2 = std::make_unique<Label>(i18n().tr("preferences.users.confirm_password") + ":");
        lbl2->set_fixed_size(45);
        root->add_child(std::move(lbl2));

        auto box2 = std::make_unique<TextBox<PasswordPolicy>>();
        m_pass2_box = box2.get();
        m_pass2_box->set_fixed_size(35);
        root->add_child(std::move(box2));

        // Error label
        auto err = std::make_unique<Label>("");
        err->set_text_color(Color(0.8f, 0.2f, 0.2f, 1.0f));
        m_error_label = err.get();
        root->add_child(Spacer(10));
        root->add_child(std::move(err));

        root->add_child(Spacer());

        // Buttons
        auto footer = std::make_unique<Widget>();
        footer->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        footer->set_fixed_size(35);
        footer->set_spacing(10);
        footer->add_child(Spacer());

        auto cancel_btn = std::make_unique<Button<AquaObject>>();
        cancel_btn->set_text(i18n().tr("preferences.common.cancel"));
        cancel_btn->set_width(100);
        cancel_btn->when_click.connect([this](auto &) { on_cancel(); });
        footer->add_child(std::move(cancel_btn));

        auto accept_btn = std::make_unique<Button<AquaObject>>();
        accept_btn->set_accent_color(WidgetAccentColor::Primary);
        accept_btn->set_text(i18n().tr("preferences.common.accept"));
        accept_btn->set_width(100);
        accept_btn->when_click.connect([this](auto &) { on_accept(); });
        footer->add_child(std::move(accept_btn));

        root->add_child(std::move(footer));

        window_widget->add_child(std::move(root));
        set_root(std::move(window_widget));
    }

    void PasswordDialog::on_accept()
    {
        std::string p1 = m_pass1_box->text();
        std::string p2 = m_pass2_box->text();

        if (p1.empty())
        {
            m_error_label->set_text(i18n().tr("preferences.users.error_empty_password"));
            return;
        }

        if (p1 != p2)
        {
            m_error_label->set_text(i18n().tr("preferences.users.error_password_mismatch"));
            return;
        }

        PasswordDialogEvent ev;
        ev.accepted = true;
        ev.password = p1;
        when_finished.run(ev);
        quit();
    }

    void PasswordDialog::on_cancel()
    {
        PasswordDialogEvent ev;
        ev.accepted = false;
        when_finished.run(ev);
        quit();
    }
} // namespace horizon::preferences
