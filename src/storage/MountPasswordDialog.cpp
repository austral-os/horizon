#include "horizon/storage/MountPasswordDialog.hpp"
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/RadioButton.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Window.hpp>

namespace horizon::storage
{
    MountPasswordDialog::MountPasswordDialog(const std::string &server_name)
        : WaylandWindow("horizon.storage.mount_dialog", 700, 410, false, false)
    {
        set_name(i18n().tr("core.storage.mount_dialog.title"));

        auto window_widget =
            std::make_unique<horizon::Window>(i18n().tr("core.storage.mount_dialog.title"));

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_spacing(15);
        content->set_margin(15);

        // Header
        auto header = std::make_unique<Widget>();
        header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header->set_fixed_size(65);

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name("network-server");
        icon->set_icon_size(64);
        icon->set_fixed_size(64);

        auto text_container = std::make_unique<Widget>();
        text_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        text_container->set_spacing(5);

        auto title = std::make_unique<Label>(i18n().tr("core.storage.mount_dialog.instruction"));
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_fixed_size(35);

        auto server_label = std::make_unique<Label>("\"" + server_name + "\"");
        server_label->set_font_weight(FONT_WEIGHT_BOLD);

        text_container->add_child(std::move(title));
        text_container->add_child(std::move(server_label));

        header->add_child(std::move(icon));
        header->add_child(Spacer(15));
        header->add_child(std::move(text_container));
        content->add_child(std::move(header));

        // Form area
        auto form = std::make_unique<Widget>();
        form->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        form->set_spacing(15);

        auto connect_as_row = std::make_unique<Widget>();
        connect_as_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        connect_as_row->set_position_type(WidgetPositionTypes::FILL);
        connect_as_row->set_fixed_size(35);

        auto lbl_connect_as =
            std::make_unique<Label>(i18n().tr("core.storage.mount_dialog.connect_as"));
        lbl_connect_as->set_fixed_size(200);

        connect_as_row->add_child(std::move(lbl_connect_as));

        auto guest_container = std::make_unique<Widget>();
        guest_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        guest_container->set_spacing(5);

        auto guest = std::make_unique<RadioButton<AquaObject>>();
        m_guest_radio = guest.get();
        guest_container->add_child(std::move(guest));
        guest_container->add_child(
            std::make_unique<Label>(i18n().tr("core.storage.mount_dialog.guest")));

        auto user_container = std::make_unique<Widget>();
        user_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        user_container->set_spacing(5);

        auto user = std::make_unique<RadioButton<AquaObject>>();
        user->set_selected(true);
        m_user_radio = user.get();
        user_container->add_child(std::move(user));
        user_container->add_child(
            std::make_unique<Label>(i18n().tr("core.storage.mount_dialog.registered_user")));

        connect_as_row->add_child(std::move(guest_container));
        connect_as_row->add_child(horizon::Spacer(15)); // Espacio entre opciones
        connect_as_row->add_child(std::move(user_container));
        form->add_child(std::move(connect_as_row));

        // Inputs
        auto name_row = std::make_unique<Widget>();
        name_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        name_row->set_fixed_size(35);

        auto name_label = std::make_unique<Label>(i18n().tr("core.storage.mount_dialog.name"));
        name_label->set_fixed_size(150);
        name_row->add_child(std::move(name_label));

        auto name = std::make_unique<TextBox<TextPolicy>>();
        name->set_placeholder(i18n().tr("core.storage.mount_dialog.username_placeholder"));
        name->set_fixed_size(-1);
        m_name_input = name.get();
        name_row->add_child(std::move(name));
        form->add_child(std::move(name_row));

        auto pass_row = std::make_unique<Widget>();
        pass_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        pass_row->set_fixed_size(35);

        auto pass_label = std::make_unique<Label>(i18n().tr("core.storage.mount_dialog.password"));
        pass_label->set_fixed_size(150);

        pass_row->add_child(std::move(pass_label));

        auto pass = std::make_unique<TextBox<PasswordPolicy>>();
        pass->set_fixed_size(-1);
        m_pass_input = pass.get();
        pass_row->add_child(std::move(pass));

        form->add_child(std::move(pass_row));

        // Error Label (Hidden by default)
        auto error_label = std::make_unique<Label>("");
        error_label->set_text_color({1.0f, 0.2f, 0.2f, 1.0f}); // Error red
        error_label->set_visible(false);
        error_label->set_height(40); // Larger height for wrapping
        m_error_label = error_label.get();
        form->add_child(std::move(error_label));

        // Loading Bar (Hidden by default)
        auto loading_bar = std::make_unique<ProgressBar>();
        loading_bar->set_indeterminate(true);
        loading_bar->set_visible(false);
        loading_bar->set_fixed_size(25);
        m_loading_bar = loading_bar.get();
        form->add_child(std::move(loading_bar));

        form->add_child(Spacer());

        auto remember = std::make_unique<Checkbox<AquaObject>>();
        remember->set_text(i18n().tr("core.storage.mount_dialog.remember"));
        remember->set_checked(true);
        m_remember_check = remember.get();
        form->add_child(std::move(remember));

        content->add_child(std::move(form));

        // Footer
        auto footer = std::make_unique<Widget>();
        footer->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        footer->set_fixed_size(35);
        footer->add_child(horizon::Spacer());

        auto cancel = std::make_unique<Button<AquaObject>>();
        cancel->set_text(i18n().tr("core.storage.mount_dialog.cancel"));
        cancel->set_fixed_size(150);
        cancel->when_click.connect([this](auto &) { quit(); });

        auto connect = std::make_unique<Button<AquaObject>>();
        connect->set_text(i18n().tr("core.storage.mount_dialog.connect"));
        connect->set_accent_color(WidgetAccentColor::Primary);
        connect->set_fixed_size(150);
        m_connect_btn = connect.get();
        connect->when_click.connect(
            [this](auto &)
            {
                show_loading();
                MountPasswordEvent ev;
                ev.credentials.is_guest = m_guest_radio->is_selected();
                ev.credentials.username = m_name_input->text();
                ev.credentials.password = m_pass_input->text();
                ev.credentials.remember = m_remember_check->is_checked();
                when_accepted.run(ev);
            });

        footer->add_child(std::move(cancel));
        footer->add_child(horizon::Spacer(10));
        footer->add_child(std::move(connect));

        content->add_child(std::move(footer));

        window_widget->add_child(std::move(content));
        set_root(std::move(window_widget));

        m_guest_radio->when_mouse_press.connect(
            [this](auto &)
            {
                m_user_radio->set_selected(false);
                update_enabled_state();
            });
        m_user_radio->when_mouse_press.connect(
            [this](auto &)
            {
                m_guest_radio->set_selected(false);
                update_enabled_state();
            });

        update_enabled_state();
    }

    void MountPasswordDialog::update_enabled_state()
    {
        if (!m_user_radio || !m_name_input || !m_pass_input || !m_remember_check)
            return;
        bool is_user = m_user_radio->is_selected();
        m_name_input->set_enabled(is_user);
        m_name_input->set_focus(is_user);
        m_pass_input->set_enabled(is_user);
        m_remember_check->set_enabled(is_user);
    }

    void MountPasswordDialog::show_loading()
    {
        LOG_INFO << "MountPasswordDialog: Activando estado de carga...";
        set_override_cursor(CursorType::Wait);
        if (m_loading_bar)
            m_loading_bar->set_visible(true);
        if (m_error_label)
            m_error_label->set_visible(false);
        if (m_connect_btn)
            m_connect_btn->set_enabled(false);

        // Disable inputs while loading
        if (m_guest_radio)
            m_guest_radio->set_enabled(false);
        if (m_user_radio)
            m_user_radio->set_enabled(false);
        if (m_name_input)
            m_name_input->set_enabled(false);
        if (m_pass_input)
            m_pass_input->set_enabled(false);
        if (m_remember_check)
            m_remember_check->set_enabled(false);
    }

    void MountPasswordDialog::show_error(const std::string &message)
    {
        LOG_INFO << "MountPasswordDialog: Mostrando error en UI: " << message;
        clear_override_cursor();
        if (m_loading_bar)
            m_loading_bar->set_visible(false);
        if (m_error_label)
        {
            m_error_label->set_text(message);
            m_error_label->set_visible(true);
        }
        if (m_connect_btn)
            m_connect_btn->set_enabled(true);

        // Re-enable inputs based on current selection
        if (m_guest_radio)
            m_guest_radio->set_enabled(true);
        if (m_user_radio)
            m_user_radio->set_enabled(true);
        update_enabled_state();
    }
} // namespace horizon::storage
