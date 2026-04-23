#pragma once

#include "GreetdClient.hpp"
#include "SessionProvider.hpp"
#include "UserProvider.hpp"
#include <horizon/Combo.hpp>
#include <horizon/CoverFlow.hpp>
#include <horizon/Image.hpp>
#include <horizon/Label.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>

namespace horizon { class Textarea; }

namespace horizon::greeter
{
    /**
     * @class GreeterWindow
     * @brief The main UI for the Horizon Greeter.
     */
    class GreeterWindow : public WaylandLayerWindow
    {
    public:
        GreeterWindow(GreetdClient &client, Application &app);
        ~GreeterWindow() override = default;

        void initialize() override;
        void on_key_event(const KeyEvent &event) override;

    private:
        void setup_ui();
        void load_data();
        void on_user_selected(int index);
        void on_login_pressed();
        void update_background(const std::string &path);

        GreetdClient &m_client;
        Application &m_app;
        std::vector<UserInfo> m_users;
        std::vector<SessionInfo> m_sessions;

        // UI Components
        Image *m_background_image{nullptr};
        CoverFlow<UserInfo> *m_user_cover_flow{nullptr};
        TextBoxBase *m_password_box{nullptr};
        Combo *m_session_combo{nullptr};
        Label *m_message_label{nullptr};
        horizon::Textarea *m_log_view{nullptr};
        Widget *m_login_container{nullptr};

        bool m_is_authenticating{false};
    };
} // namespace horizon::greeter
