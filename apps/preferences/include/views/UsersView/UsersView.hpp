#pragma once
#include <horizon/AvatarSelector.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Label.hpp>
#include <horizon/Sidebar.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Widget.hpp>
#include <string>
#include <vector>

namespace horizon::preferences
{
    struct UserInfo
    {
        std::string username;
        std::string real_name;
        std::string email;
        std::string avatar_path;
        bool is_admin{false};
        bool is_current{false};
        uid_t uid{0};
    };

    class UsersView : public Widget
    {
    public:
        UsersView();
        ~UsersView() override = default;

    private:
        void setup_ui();
        void load_users();
        void select_user(const UserInfo &user);

        // Actions
        void on_add_user_clicked();
        void on_save_clicked();
        void on_change_password_clicked();
        void on_delete_user_clicked();

        // Helpers
        void open_password_dialog(const std::string &username);
        std::string get_user_avatar(const std::string &username);
        bool check_is_admin(const std::string &username);

        // UI Members
        Sidebar *m_sidebar{nullptr};
        Widget *m_editor_pane{nullptr};

        // Editor Fields
        AvatarSelector *m_avatar_selector{nullptr};
        TextBoxBase *m_fullname_box{nullptr};
        TextBoxBase *m_username_box{nullptr};
        TextBoxBase *m_email_box{nullptr};
        Combo *m_account_type_combo{nullptr};

        Button<AquaObject> *m_save_btn{nullptr};
        Button<SolidObject> *m_password_btn{nullptr};
        Button<SolidObject> *m_delete_btn{nullptr};

        std::vector<UserInfo> m_users;
        UserInfo m_selected_user;
        UserInfo m_current_user;
        bool m_is_new_user{false};
    };
} // namespace horizon::preferences
