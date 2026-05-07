#include <algorithm>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/VPanel.hpp>
#include <pwd.h>
#include <sstream>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <views/UsersView/PasswordDialog.hpp>
#include <views/UsersView/UsersView.hpp>

namespace fs = std::filesystem;
 
namespace horizon::preferences
{
    static std::string shell_escape(const std::string &s)
    {
        std::string res = "'";
        for (char c : s)
        {
            if (c == '\'')
                res += "'\\''";
            else
                res += c;
        }
        res += "'";
        return res;
    }
    // Custom Sidebar Item for Users
    class UserSidebarItem : public SidebarItem
    {
    public:
        UserSidebarItem(const UserInfo &user)
            : SidebarItem(user.avatar_path.empty() ? "user-identity" : user.avatar_path,
                          user.real_name),
              m_user(user)
        {
            set_fixed_size(60);

            // Hide default components from SidebarItem
            if (m_icon_ptr)
                m_icon_ptr->set_visible(false);
            if (m_label_ptr)
                m_label_ptr->set_visible(false);

            set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            set_spacing(15);
            set_margin(10);

            auto avatar = std::make_unique<Icon>();
            avatar->set_icon_name(user.avatar_path.empty() ? "user-identity" : user.avatar_path);
            avatar->set_icon_size(40);
            avatar->set_fixed_size(40);
            add_child(std::move(avatar));

            auto text_container = std::make_unique<Widget>();
            text_container->set_position_type(FILL);
            text_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            text_container->set_spacing(2);

            auto name_label = std::make_unique<Label>(user.real_name);
            name_label->set_font_weight(FONT_WEIGHT_BOLD);
            name_label->set_font_size(13);
            m_name_label = name_label.get();
            text_container->add_child(std::move(name_label));

            auto user_label = std::make_unique<Label>(user.username);
            user_label->set_font_size(11);
            user_label->set_text_color(Color(0.5f, 0.5f, 0.5f, 1.0f));
            m_user_label = user_label.get();
            text_container->add_child(std::move(user_label));

            add_child(std::move(text_container));
        }

        const UserInfo &user_info() const
        {
            return m_user;
        }

        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override
        {
            update_styles();
            SidebarItem::render(gc, cx, cy, cw, ch, force);
        }

        void draw(GraphicsContext &gc) override
        {
            if (is_selected())
            {
                gc.setColor(Color("#3498db"));
                gc.fillRect(m_x + 5, m_y + 2, m_width - 10, m_height - 4, CornerRadius(8));
            }
            else if (is_hovered())
            {
                gc.setColor(Color(0.0f, 0.0f, 0.0f, 0.05f));
                gc.fillRect(m_x + 5, m_y + 2, m_width - 10, m_height - 4, CornerRadius(8));
            }
        }

    private:
        void update_styles()
        {
            if (is_selected())
            {
                m_name_label->set_text_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
                m_user_label->set_text_color(Color(0.9f, 0.9f, 0.9f, 1.0f));
            }
            else
            {
                m_name_label->set_text_color(Color(0.2f, 0.2f, 0.2f, 1.0f));
                m_user_label->set_text_color(Color(0.5f, 0.5f, 0.5f, 1.0f));
            }
        }

        UserInfo m_user;
        Label *m_name_label{nullptr};
        Label *m_user_label{nullptr};
    };

    UsersView::UsersView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);

        setup_ui();
        load_users();
    }

    void UsersView::setup_ui()
    {
        // 1. Sidebar (Left)
        auto sidebar = std::make_unique<Sidebar>();
        m_sidebar = sidebar.get();
        m_sidebar->set_fixed_size(280);
        m_sidebar->set_position_type(FILL);

        m_sidebar->when_item_selected.connect(
            [this](SidebarItemSelectedContext &ctx)
            {
                auto *user_item = dynamic_cast<UserSidebarItem *>(ctx.item);
                if (user_item)
                {
                    select_user(user_item->user_info());
                }
            });

        add_child(std::move(sidebar));

        // 2. Editor Pane (Right)
        auto editor = std::make_unique<Widget>();
        m_editor_pane = editor.get();
        m_editor_pane->set_position_type(FILL);
        m_editor_pane->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        m_editor_pane->set_margin(40);
        m_editor_pane->set_spacing(20);

        // Large Avatar
        auto avatar_container = std::make_unique<Widget>();
        avatar_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        avatar_container->set_fixed_size(120);
        avatar_container->add_child(Spacer());

        auto avatar_sel = std::make_unique<AvatarSelector>();
        m_avatar_selector = avatar_sel.get();
        m_avatar_selector->set_fixed_size(110);
        avatar_container->add_child(std::move(avatar_sel));

        avatar_container->add_child(Spacer());
        m_editor_pane->add_child(std::move(avatar_container));

        // Form fields
        auto add_row = [&](const std::string &label_text, std::unique_ptr<Widget> field) -> Widget *
        {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(35);
            row->set_spacing(10);

            auto label = std::make_unique<Label>(label_text);
            label->set_fixed_size(180);
            label->set_alignment(TextAlignment::Right);
            label->set_vertical_alignment(VerticalAlignment::Middle);
            row->add_child(std::move(label));

            auto field_ptr = field.get();
            field_ptr->set_position_type(FILL);
            row->add_child(std::move(field));

            m_editor_pane->add_child(std::move(row));
            return field_ptr;
        };

        auto fullname_box = std::make_unique<TextBox<TextPolicy>>();
        fullname_box->set_fixed_size(-1);
        m_fullname_box = fullname_box.get();
        add_row(i18n().tr("preferences.users.name") + ":", std::move(fullname_box));

        auto username_box = std::make_unique<TextBox<TextPolicy>>();
        username_box->set_fixed_size(-1);
        m_username_box = username_box.get();
        add_row(i18n().tr("preferences.users.username") + ":", std::move(username_box));

        auto account_type = std::make_unique<Combo>();
        m_account_type_combo = account_type.get();
        m_account_type_combo->add_item("standard", i18n().tr("preferences.users.type_standard"));
        m_account_type_combo->add_item("admin", i18n().tr("preferences.users.type_admin"));
        add_row(i18n().tr("preferences.users.account_type") + ":", std::move(account_type));

        auto email_box = std::make_unique<TextBox<TextPolicy>>();
        email_box->set_fixed_size(-1);
        m_email_box = email_box.get();
        add_row(i18n().tr("preferences.users.email") + ":", std::move(email_box));

        // Buttons
        auto btn_row = std::make_unique<Widget>();
        btn_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_row->set_fixed_size(35);
        btn_row->set_spacing(10);
        btn_row->add_child(Spacer(20));

        auto pass_btn = std::make_unique<Button<SolidObject>>();
        pass_btn->set_text(i18n().tr("preferences.users.change_password"));
        pass_btn->when_click.connect([this](auto &) { on_change_password_clicked(); });
        m_password_btn = pass_btn.get();

        btn_row->add_child(std::move(pass_btn));

        btn_row->add_child(Spacer(20));

        // Delete Button
        auto del_btn = std::make_unique<Button<SolidObject>>();
        del_btn->set_text(i18n().tr("preferences.users.delete_user"));
        del_btn->set_text_color(Color(0.8f, 0.2f, 0.2f, 1.0f));
        del_btn->when_click.connect([this](auto &) { on_delete_user_clicked(); });
        m_delete_btn = del_btn.get();
        btn_row->add_child(std::move(del_btn));

        m_editor_pane->add_child(std::move(btn_row));

        m_editor_pane->add_child(Spacer());

        // Save Button
        auto footer = std::make_unique<Widget>();
        footer->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        footer->set_fixed_size(40);
        footer->add_child(Spacer());

        auto save_btn = std::make_unique<Button<AquaObject>>();
        save_btn->set_accent_color(WidgetAccentColor::Primary);
        save_btn->set_text(i18n().tr("preferences.buttons.save"));
        save_btn->set_fixed_size(120);
        save_btn->when_click.connect([this](auto &) { on_save_clicked(); });
        m_save_btn = save_btn.get();
        footer->add_child(std::move(save_btn));

        m_editor_pane->add_child(std::move(footer));

        add_child(std::move(editor));
    }

    void UsersView::load_users()
    {
        m_sidebar->clear();
        m_users.clear();
        struct passwd *pw;
        uid_t current_uid = getuid();

        setpwent();
        while ((pw = getpwent()) != nullptr)
        {
            if (pw->pw_uid < 1000 || pw->pw_uid > 60000)
                continue;

            UserInfo user;
            user.username = pw->pw_name ? pw->pw_name : "";
            user.uid = pw->pw_uid;
            
            std::string gecos = pw->pw_gecos ? pw->pw_gecos : "";
            std::vector<std::string> fields;
            std::stringstream ss(gecos);
            std::string segment;
            while (std::getline(ss, segment, ',')) {
                fields.push_back(segment);
            }
            
            user.real_name = !fields.empty() ? fields[0] : user.username;
            if (user.real_name.empty()) user.real_name = user.username;
            
            // Email is often stored in the 5th field of GECOS
            if (fields.size() >= 5) {
                user.email = fields[4];
            }

            user.avatar_path = get_user_avatar(user.username);
            user.is_admin = check_is_admin(user.username);
            user.is_current = (pw->pw_uid == current_uid);

            if (user.is_current)
                m_current_user = user;
            m_users.push_back(user);
        }
        endpwent();

        // Sort: Current first, then alphabetical
        std::sort(m_users.begin(), m_users.end(),
                  [](const UserInfo &a, const UserInfo &b)
                  {
                      if (a.is_current)
                          return true;
                      if (b.is_current)
                          return false;
                      return a.username < b.username;
                  });

        // Add to sidebar
        m_sidebar->add_group(i18n().tr("preferences.users.your_account"));
        m_sidebar->add_group(i18n().tr("preferences.users.other_accounts"));

        UserSidebarItem* first_item = nullptr;
        for (const auto &user : m_users)
        {
            std::string group = user.is_current ? i18n().tr("preferences.users.your_account")
                                                : i18n().tr("preferences.users.other_accounts");
            auto item = std::make_unique<UserSidebarItem>(user);
            if (!first_item) first_item = item.get();
            m_sidebar->add_item(group, std::move(item));
        }

        if (first_item) {
            m_sidebar->select_item(first_item);
            select_user(first_item->user_info());
        }
    }

    void UsersView::select_user(const UserInfo &user)
    {
        m_selected_user = user;

        m_fullname_box->set_text(user.real_name);
        m_username_box->set_text(user.username);
        m_email_box->set_text(user.email);
        m_account_type_combo->set_selected_item_by_id(user.is_admin ? "admin" : "standard");
        m_avatar_selector->set_selected_avatar(user.avatar_path);

        m_delete_btn->set_enabled(!user.is_current);
    }

    void UsersView::on_save_clicked()
    {
        std::string new_fullname = m_fullname_box->text();
        std::string new_username = m_username_box->text();
        std::string new_email = m_email_box->text();
        bool new_is_admin = false;
        if (m_account_type_combo->selected_item())
        {
            new_is_admin = (m_account_type_combo->selected_item()->id == "admin");
        }

        std::string cmd = "";

        if (new_fullname != m_selected_user.real_name || new_is_admin != m_selected_user.is_admin || new_email != m_selected_user.email)
        {
            cmd = "/usr/sbin/usermod ";
            if (new_fullname != m_selected_user.real_name || new_email != m_selected_user.email)
            {
                // Format GECOS: Full Name,Room,Work,Home,Email
                cmd += "-c \"" + new_fullname + ",,,," + new_email + "\" ";
            }
            if (new_is_admin != m_selected_user.is_admin)
            {
                cmd += "-G " + std::string(new_is_admin ? "sudo" : "users") + " ";
            }
            cmd += m_selected_user.username;
        }

        if (!cmd.empty())
        {
            cmd = "pkexec " + cmd;
            LOG_INFO << "Executing: " << cmd;
            std::system(cmd.c_str());
            load_users();
        }
    }

    void UsersView::on_change_password_clicked()
    {
        std::thread([this, username = m_selected_user.username]() {
            auto dialog = std::make_unique<PasswordDialog>(username);
            dialog->when_finished.connect(
                [this, username](PasswordDialogEvent &ev)
                {
                    if (ev.accepted && !ev.password.empty())
                    {
                        // Use chpasswd to set password non-interactively
                        std::string payload = username + ":" + ev.password;
                        std::string cmd = "printf %s " + shell_escape(payload) + " | ";
                        if (username != m_current_user.username)
                            cmd += "pkexec ";

                        cmd += "/usr/sbin/chpasswd";
 
                        LOG_INFO << "Updating password for " << username << " using chpasswd";
                        std::system(cmd.c_str());
                    }
                });
            dialog->run();
        }).detach();
    }

    void UsersView::on_delete_user_clicked()
    {
        if (m_selected_user.is_current)
            return;
        std::string cmd = "pkexec /usr/sbin/userdel -r " + m_selected_user.username;
        LOG_INFO << "Deleting user: " << cmd;
        std::system(cmd.c_str());
        load_users();
    }

    std::string UsersView::get_user_avatar(const std::string &username)
    {
        std::error_code ec;
        std::string path = "/home/" + username + "/.face";
        if (fs::exists(path, ec))
            return path;

        path = "/var/lib/AccountsService/users/" + username;
        if (fs::exists(path, ec))
        {
            std::ifstream f(path);
            std::string line;
            while (std::getline(f, line))
            {
                if (line.find("Icon=") == 0)
                    return line.substr(5);
            }
        }
        return "";
    }

    bool UsersView::check_is_admin(const std::string &username)
    {
        struct group *gr = getgrnam("sudo");
        if (!gr)
            gr = getgrnam("wheel");
        if (!gr)
            return false;

        for (int i = 0; gr->gr_mem[i] != nullptr; i++)
        {
            if (username == gr->gr_mem[i])
                return true;
        }
        return false;
    }

} // namespace horizon::preferences
