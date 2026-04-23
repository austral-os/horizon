#include "GreeterWindow.hpp"
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Logger.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Textarea.hpp>
#include <string>
#include <filesystem>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>
#include <fstream>

namespace fs = std::filesystem;

namespace horizon::greeter
{
    /**
     * @class UserItem
     * @brief Individual user widget for CoverFlow.
     */
    class UserItem : public Widget
    {
    public:
        UserItem(const UserInfo &info, bool selected)
        {
            set_layout_type(WIDGET_LAYOUT_VERTICAL);
            set_fixed_size(180);

            auto icon = std::make_unique<Icon>();
            icon->set_icon_name("avatar-default");
            icon->set_icon_size(128);
            icon->set_fixed_size(128);
            add_child(std::move(icon));

            auto name = std::make_unique<Label>(info.real_name);
            name->set_alignment(TextAlignment::Center);
            name->set_text_color(Color(1.0f, 1.0f, 1.0f));
            name->set_font_size(selected ? 18 : 14);
            add_child(std::move(name));
        }
    };

    GreeterWindow::GreeterWindow(GreetdClient &client, Application &app, bool debug)
        : WaylandLayerWindow("horizon-greeter", 3, true), m_client(client), m_app(app), m_debug(debug)
    {
    }

    void GreeterWindow::initialize()
    {
        WaylandLayerWindow::initialize();
        set_anchor(0xF);               // Anchor to all sides
        set_keyboard_interactivity(1); // On

        setup_ui();
        ensure_gtk_icon_theme();
        load_data();

        // Connect greetd signals
        m_client.on_auth_message = [this](const GreetdAuthMessage &msg)
        {
            if (msg.type == "secret")
            {
                m_password_box->set_visible(true);
                m_password_box->set_focus(true);
            }
            m_message_label->set_text(msg.message);
        };

        m_client.on_error = [this](const std::string &type, const std::string &desc)
        {
            m_app.alert(desc, "Login Error", MessageType::Error);
            m_message_label->set_text("Error: " + desc);
            m_message_label->set_text_color(Color(1.0f, 0.4f, 0.4f));
            m_is_authenticating = false;
            m_password_box->set_enabled(true);
        };

        m_client.on_success = [this]()
        {
            LOG_INFO << "GreeterWindow: Authentication successful.";
            m_message_label->set_text("Login successful!");
            m_message_label->set_text_color(Color(0.4f, 1.0f, 0.4f));

            // Start the selected session
            if (m_session_combo && m_session_combo->selected_item_index() != -1)
            {
                const auto &session = m_sessions[m_session_combo->selected_item_index()];
                LOG_INFO << "GreeterWindow: Starting session: " << session.name << " (" << session.exec << ")";
                std::vector<std::string> cmd = {"/bin/sh", "-c", session.exec};
                m_client.start_session(cmd);

                // Give it a tiny moment to send the message before quitting
                this->quit();
            }
            else
            {
                LOG_ERROR << "GreeterWindow: No session selected!";
                m_message_label->set_text("Error: No session selected");
            }
        };

        m_client.on_reset = [this]()
        {
            m_is_authenticating = false;
            m_password_box->set_enabled(true);
            m_password_box->set_text("");
            m_message_label->set_text("");
        };

        // Redirect logs to UI (Only Greetd related logs)
        horizon::Logger::instance().set_callback([this](horizon::LogLevel level, const std::string &msg) {
            std::string lower_msg = msg;
            for (auto &c : lower_msg) c = (char)std::tolower(c);

            if (m_log_view && lower_msg.find("greetd") != std::string::npos)
            {
                std::string prefix = "[INFO] ";
                if (level == horizon::LogLevel::WARNING) prefix = "[WARN] ";
                if (level == horizon::LogLevel::ERROR) prefix = "[ERR ] ";
                m_log_view->set_text(m_log_view->text() + prefix + msg + "\n");
                m_log_view->move_cursor_to_end();
            }
        });
    }

    void GreeterWindow::on_key_event(const KeyEvent &event)
    {
        if (event.type == KeyEvent::Type::Press) // Press
        {
            // Direct capture of Enter key
            if (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter)
            {
                on_login_pressed();
                return;
            }

            // If user is typing password, don't change user with arrows
            if (!(m_password_box->has_focus() && !m_password_box->text().empty()))
            {
                if (event.keysym == XKB_KEY_Left)
                {
                    int idx = m_user_cover_flow->selected_index();
                    if (idx > 0)
                    {
                        m_user_cover_flow->set_selected_index(idx - 1);
                        EventContext ev;
                        m_user_cover_flow->when_selection_changed.run(ev);
                    }
                }
                else if (event.keysym == XKB_KEY_Right)
                {
                    int idx = m_user_cover_flow->selected_index();
                    if (idx < (int)m_users.size() - 1)
                    {
                        m_user_cover_flow->set_selected_index(idx + 1);
                        EventContext ev;
                        m_user_cover_flow->when_selection_changed.run(ev);
                    }
                }
            }
        }

        WaylandLayerWindow::on_key_event(event);
    }

    void GreeterWindow::setup_ui()
    {
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root->set_position_type(FILL);

        // 1. Background Image (fills everything)
        auto bg = std::make_unique<Image>();
        bg->set_position_type(FREE);
        bg->set_size(width(), height());
        bg->set_mode(ImageMode::Stretch);
        m_background_image = bg.get();
        root->add_child(std::move(bg));

        // 2. Main Content Container
        auto main_container = std::make_unique<Widget>();
        main_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        main_container->set_position_type(FILL);
        main_container->set_background_color(
            Color(0.0f, 0.0f, 0.0f, 0.4f)); // Semi-transparent overlay

        main_container->add_child(Spacer());

        // CoverFlow
        auto cf = std::make_unique<CoverFlow<UserInfo>>();
        m_user_cover_flow = cf.get();
        m_user_cover_flow->set_fixed_size(300);
        m_user_cover_flow->set_item_factory([](const UserInfo &info, bool selected)
                                            { return std::make_unique<UserItem>(info, selected); });
        m_user_cover_flow->set_draw_background(false);
        m_user_cover_flow->set_draw_reflection(false);
        m_user_cover_flow->when_selection_changed.connect(
            [this](auto &) { on_user_selected(m_user_cover_flow->selected_index()); });
        main_container->add_child(std::move(cf));

        main_container->add_child(Spacer(20));

        // Login Area
        auto login_area = std::make_unique<Widget>();
        login_area->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        login_area->set_fixed_size(150);
        login_area->set_spacing(10);

        auto pass_row = std::make_unique<Widget>();
        pass_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        pass_row->set_fixed_size(40);
        pass_row->add_child(Spacer());

        auto pass_box = std::make_unique<TextBox<PasswordPolicy>>();
        m_password_box = pass_box.get();
        m_password_box->set_fixed_size(300);
        m_password_box->set_placeholder("Password");
        m_password_box->set_visible(false);
        pass_row->add_child(std::move(pass_box));
        pass_row->add_child(Spacer());
        login_area->add_child(std::move(pass_row));

        auto msg = std::make_unique<Label>("");
        m_message_label = msg.get();
        m_message_label->set_alignment(TextAlignment::Center);
        m_message_label->set_text_color(Color(1.0f, 1.0f, 1.0f));
        m_message_label->set_fixed_size(30);
        login_area->add_child(std::move(msg));

        main_container->add_child(std::move(login_area));
        main_container->add_child(Spacer());

        // Footer
        auto footer = std::make_unique<Widget>();
        footer->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        footer->set_fixed_size(110);
        footer->set_margin(20);

        // Power buttons
        auto shutdown_btn = std::make_unique<horizon::ToolbarButton>("Shutdown", "system-shutdown");
        shutdown_btn->set_fixed_size(80);
        shutdown_btn->when_click.connect([](auto &) { system("systemctl poweroff"); });
        footer->add_child(std::move(shutdown_btn));
        footer->add_child(Spacer(10));

        auto reboot_btn = std::make_unique<horizon::ToolbarButton>("Reboot", "system-reboot");
        reboot_btn->set_fixed_size(80);
        reboot_btn->when_click.connect([](auto &) { system("systemctl reboot"); });
        footer->add_child(std::move(reboot_btn));

        footer->add_child(Spacer());

        // Session Combo
        auto combo = std::make_unique<Combo>();
        m_session_combo = combo.get();
        m_session_combo->set_fixed_size(200);
        m_session_combo->set_visible(m_debug); // Only visible in debug mode
        footer->add_child(std::move(combo));

        main_container->add_child(std::move(footer));

        // 3. Log View (at the bottom)
        auto log_area = std::make_unique<horizon::Textarea>();
        m_log_view = log_area.get();
        m_log_view->set_height(150); 
        m_log_view->set_background_color(Color(0.0f, 0.0f, 0.0f, 0.6f));
        m_log_view->set_visible(m_debug); // Only visible in debug mode
        main_container->add_child(std::move(log_area));

        root->add_child(std::move(main_container));
        set_root(std::move(root));
    }

    void GreeterWindow::load_data()
    {
        m_users = UserProvider::get_users();
        m_user_cover_flow->set_data(m_users);

        m_sessions = SessionProvider::get_sessions();
        for (const auto &s : m_sessions)
        {
            m_session_combo->add_item(s.name, s.name);
        }
        if (!m_sessions.empty())
        {
            int selected_idx = 0;
            for (int i = 0; i < (int)m_sessions.size(); ++i)
            {
                if (m_sessions[i].name == "Horizon")
                {
                    selected_idx = i;
                    break;
                }
            }
            m_session_combo->set_selected_item_index(selected_idx);
        }

        if (!m_users.empty())
        {
            on_user_selected(0);
            m_user_cover_flow->set_selected_index(0);
        }
    }

    void GreeterWindow::on_user_selected(int index)
    {
        if (index < 0 || index >= (int)m_users.size())
            return;

        const auto &user = m_users[index];
        
        // Prevent redundant calls to create_session
        if (user.username == m_current_username)
            return;
            
        m_current_username = user.username;
        update_background(user.wallpaper_path);

        // Initiate auth session with greetd
        m_client.create_session(user.username);

        m_password_box->set_text("");
        m_password_box->set_visible(true);
        m_password_box->set_focus(true);
        m_message_label->set_text("Enter password for " + user.real_name);
    }

    void GreeterWindow::on_login_pressed()
    {
        if (m_is_authenticating)
            return;

        if (!m_client.is_connected())
        {
            LOG_WARNING << "GreeterWindow: Not connected to greetd.";
            m_message_label->set_text("ERROR: Sin conexión a greetd");
            return;
        }

        m_is_authenticating = true;
        m_message_label->set_text("Verificando...");
        m_password_box->set_enabled(false);
        m_client.post_auth_message_response(m_password_box->text());
    }

    void GreeterWindow::update_background(const std::string &path)
    {
        if (!m_background_image)
            return;

        std::string final_path = path;
        const std::string default_bg = "/usr/share/horizon/backgrounds/pictures/Wave.png";

        if (final_path.empty() || !std::filesystem::exists(final_path))
        {
            LOG_INFO << "GreeterWindow: Background not found or empty, using default: " << default_bg;
            final_path = default_bg;
        }

        if (std::filesystem::exists(final_path))
        {
            m_background_image->set_path(final_path);
        }
        else
        {
            LOG_ERROR << "GreeterWindow: Default background not found either: " << default_bg;
        }
    }

    void GreeterWindow::ensure_gtk_icon_theme()
    {
        const char *home = std::getenv("HOME");
        if (!home) return;

        std::vector<std::string> versions = {"3.0", "4.0"};
        for (const auto &v : versions)
        {
            std::string config_dir = std::string(home) + "/.config/gtk-" + v;
            std::string settings_path = config_dir + "/settings.ini";

            try
            {
                if (!fs::exists(config_dir))
                {
                    fs::create_directories(config_dir);
                }

                bool has_settings_section = false;
                bool has_theme_entry = false;
                std::vector<std::string> lines;

                if (fs::exists(settings_path))
                {
                    std::ifstream in(settings_path);
                    std::string line;
                    while (std::getline(in, line))
                    {
                        std::string trimmed = line;
                        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
                        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

                        if (trimmed == "[Settings]")
                        {
                            has_settings_section = true;
                        }

                        if (trimmed.rfind("gtk-icon-theme-name", 0) == 0)
                        {
                            lines.push_back("gtk-icon-theme-name=austral");
                            has_theme_entry = true;
                        }
                        else
                        {
                            lines.push_back(line);
                        }
                    }
                }

                if (!has_theme_entry)
                {
                    if (!has_settings_section)
                    {
                        lines.push_back("[Settings]");
                    }
                    lines.push_back("gtk-icon-theme-name=austral");
                }

                LOG_INFO << "GreeterWindow: Updating GTK " << v << " settings in: " << settings_path;
                std::ofstream out(settings_path);
                for (const auto &l : lines)
                {
                    out << l << "\n";
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "GreeterWindow: Error updating GTK settings: " << e.what();
            }
        }
    }
} // namespace horizon::greeter
