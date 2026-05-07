#include "UserPage.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/AvatarSelector.hpp>

namespace horizon::installer
{
    UserPage::UserPage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto title = std::make_unique<Label>(i18n().tr("installer.user.title"));
        title->set_font_size(32);
        title->set_fixed_size(60);
        add_child(std::move(title));

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_margin(20);

        // User Icon (now AvatarSelector)
        auto icon_row = std::make_unique<Widget>();
        icon_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        icon_row->set_fixed_size(120);
        icon_row->add_child(Spacer());
 
        auto avatar_sel = std::make_unique<horizon::AvatarSelector>();
        m_avatar_selector = avatar_sel.get();
        m_avatar_selector->set_fixed_size(110);
        icon_row->add_child(std::move(avatar_sel));
 
        icon_row->add_child(Spacer());
        content->add_child(std::move(icon_row));
        content->add_child(Spacer(20));

        auto add_entry = [&](const std::string &label_text, bool password = false) -> TextBoxBase *
        {
            auto label = std::make_unique<Label>(label_text);
            label->set_fixed_size(25);
            content->add_child(std::move(label));

            std::unique_ptr<TextBoxBase> box;
            if (password)
                box = std::make_unique<TextBox<PasswordPolicy>>();
            else
                box = std::make_unique<TextBox<TextPolicy>>();

            box->set_fixed_size(35);
            auto *ptr = box.get();
            ptr->when_text_changed.connect([this](auto &) { validate_inputs(); });
            content->add_child(std::move(box));
            content->add_child(Spacer(15));
            return ptr;
        };

        m_fullname_box = add_entry(i18n().tr("installer.user.fullname") + ":");
        m_username_box = add_entry(i18n().tr("installer.user.username") + ":");
        m_password_box = add_entry(i18n().tr("installer.user.password") + ":", true);
        m_verify_box = add_entry(i18n().tr("installer.user.verify") + ":", true);

        auto error_label = std::make_unique<Label>("");
        m_error_label = error_label.get();
        m_error_label->set_text_color(Color(1.0f, 0.4f, 0.4f));
        m_error_label->set_fixed_size(30);
        content->add_child(std::move(error_label));

        content->add_child(Spacer());

        add_child(std::move(content));

        auto footer = std::make_unique<Widget>();
        footer->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        footer->set_fixed_size(60);

        auto back_btn =
            std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.back"), "go-previous");
        back_btn->when_click.connect(
            [this](auto &)
            {
                EventContext ctx;
                when_back.run(ctx);
            });
        footer->add_child(std::move(back_btn));

        footer->add_child(Spacer());

        auto next_btn =
            std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.continue"), "go-next");
        m_next_btn = next_btn.get();
        m_next_btn->set_enabled(false);
        m_next_btn->when_click.connect(
            [this](auto &)
            {
                EventContext ctx;
                when_continue.run(ctx);
            });

        footer->add_child(std::move(next_btn));

        add_child(std::move(footer));
    }

    std::string UserPage::fullname() const
    {
        return static_cast<TextBoxBase *>(m_fullname_box)->text();
    }
    std::string UserPage::username() const
    {
        return static_cast<TextBoxBase *>(m_username_box)->text();
    }
    std::string UserPage::password() const
    {
        return static_cast<TextBoxBase *>(m_password_box)->text();
    }
    std::string UserPage::avatar() const
    {
        return m_avatar_selector->selected_avatar();
    }

    void UserPage::validate_inputs()
    {
        std::string user = username();
        std::string pass = password();
        std::string verify = static_cast<TextBoxBase *>(m_verify_box)->text();

        bool valid = true;
        std::string error = "";

        if (user.empty())
        {
            valid = false;
        }
        else if (pass.length() < 4)
        {
            valid = false;
            if (!pass.empty())
                error = "Password too short";
        }
        else if (pass != verify)
        {
            valid = false;
            error = "Passwords do not match";
        }

        m_error_label->set_text(error);
        m_next_btn->set_enabled(valid);
    }
} // namespace horizon::installer
