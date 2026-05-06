#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Window.hpp>
#include <pwd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <atomic>
#include <thread>
#include <vector>

using namespace horizon;

static std::atomic<bool> g_unlocked{false};

// PAM Definitions (since headers are missing)
#define PAM_SUCCESS 0
#define PAM_PROMPT_ECHO_OFF 1

struct pam_message
{
    int msg_style;
    const char *msg;
};

struct pam_response
{
    char *resp;
    int ret_code;
};

struct pam_conv
{
    int (*conv)(int num_msg, const struct pam_message **msg, struct pam_response **resp,
                void *appdata_ptr);
    void *appdata_ptr;
};

struct pam_handle;

typedef int (*pam_start_t)(const char *, const char *, const struct pam_conv *,
                           struct pam_handle **);
typedef int (*pam_authenticate_t)(struct pam_handle *, int);
typedef int (*pam_end_t)(struct pam_handle *, int);

static int pam_conversation_fn(int num_msg, const struct pam_message **msg,
                               struct pam_response **resp, void *appdata_ptr)
{
    const char *password = static_cast<const char *>(appdata_ptr);
    struct pam_response *responses =
        (struct pam_response *)calloc(num_msg, sizeof(struct pam_response));
    if (!responses)
        return 1;

    for (int i = 0; i < num_msg; ++i)
    {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF)
        {
            responses[i].resp = strdup(password);
        }
        else
        {
            responses[i].resp = nullptr;
        }
        responses[i].ret_code = 0;
    }
    *resp = responses;
    return PAM_SUCCESS;
}

bool validate_password(const std::string &username, const std::string &password)
{
    if (password.empty() || username.empty())
        return false;
    void *pam_lib = dlopen("libpam.so.0", RTLD_LAZY);
    if (!pam_lib)
        pam_lib = dlopen("/lib/x86_64-linux-gnu/libpam.so.0", RTLD_LAZY);
    if (!pam_lib)
        return false;

    auto f_pam_start = (pam_start_t)dlsym(pam_lib, "pam_start");
    auto f_pam_authenticate = (pam_authenticate_t)dlsym(pam_lib, "pam_authenticate");
    auto f_pam_end = (pam_end_t)dlsym(pam_lib, "pam_end");

    if (!f_pam_start || !f_pam_authenticate || !f_pam_end)
    {
        dlclose(pam_lib);
        return false;
    }

    struct pam_conv conv = {pam_conversation_fn, (void *)password.c_str()};
    struct pam_handle *pamh = nullptr;
    int ret = f_pam_start("login", username.c_str(), &conv, &pamh);
    if (ret != PAM_SUCCESS)
    {
        dlclose(pam_lib);
        return false;
    }

    ret = f_pam_authenticate(pamh, 0);
    bool success = (ret == PAM_SUCCESS);
    f_pam_end(pamh, ret);
    dlclose(pam_lib);
    return success;
}

class LockWindow : public WaylandLayerWindow
{
public:
    LockWindow(int monitor_index = -1) : WaylandLayerWindow("horizon.lock", 3, true, monitor_index)
    {
        set_name("Horizon Lock Screen");
        set_anchor(15); // Top, Bottom, Left, Right
        set_exclusive_zone(-1); // Ignore exclusive zones (like the dock)
        set_keyboard_interactivity(1);
        set_blur(true);

        initialize();
        setup_ui();

        // Timer to check if we should quit (unlocked from another monitor)
        add_timer(100, [this]() {
            if (g_unlocked) quit();
        }, true);
    }

private:
    void setup_ui()
    {
        // Plain Widget as root for a cleaner, full-screen overlay look
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root->set_background_color(Color(0.01f, 0.01f, 0.01f, 0.9f));

        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(25);
        container->set_width(400);

        // Large user avatar
        auto avatar = std::make_unique<Icon>();
        avatar->set_icon_name("avatar-default-symbolic");
        avatar->set_icon_size(128);
        avatar->set_fixed_size(128);
        avatar->set_horizontal_alignment(TextAlignment::Center);

        // Display name
        struct passwd *pw = getpwuid(getuid());
        std::string display_name = "User";
        if (pw)
        {
            if (pw->pw_gecos && strlen(pw->pw_gecos) > 0)
            {
                char *gecos = strdup(pw->pw_gecos);
                display_name = strtok(gecos, ",");
                free(gecos);
            }
            else
            {
                display_name = pw->pw_name;
            }
        }

        auto name_label = std::make_unique<Label>(display_name);
        name_label->set_font_size(32);
        name_label->set_font_weight(FONT_WEIGHT_BOLD);
        name_label->set_alignment(TextAlignment::Center);
        name_label->set_text_color(Color(0.95f, 0.95f, 0.95f, 1.0f));

        // Password input field
        auto password_box = std::make_unique<TextBox<PasswordPolicy>>();
        password_box->set_placeholder(i18n().tr("core.polkit.enter_password"));
        password_box->set_fixed_size(320);
        password_box->set_focusable(true);
        m_password_entry = password_box.get();

        m_password_entry->when_key_press.connect(
            [this](KeyEventContext &ev)
            {
                if (ev.keysym == XKB_KEY_Return || ev.keysym == XKB_KEY_KP_Enter)
                {
                    on_unlock();
                }
            });

        // Error message
        auto error_label = std::make_unique<Label>("");
        error_label->set_text_color(Color(1.0f, 0.4f, 0.4f, 1.0f));
        error_label->set_alignment(TextAlignment::Center);
        error_label->set_height(30);
        m_error_label = error_label.get();

        container->add_child(std::move(avatar));
        container->add_child(std::move(name_label));
        container->add_child(Spacer(10));

        auto pass_row = std::make_unique<Widget>();
        pass_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        pass_row->set_fixed_size(35);
        pass_row->add_child(Spacer());
        pass_row->add_child(std::move(password_box));
        pass_row->add_child(Spacer());
        container->add_child(std::move(pass_row));
        container->add_child(std::move(error_label));

        // Center everything
        root->add_child(Spacer());
        auto center_row = std::make_unique<Widget>();
        center_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        center_row->add_child(Spacer());
        center_row->add_child(std::move(container));
        center_row->add_child(Spacer());
        root->add_child(std::move(center_row));
        root->add_child(Spacer());

        set_root(std::move(root));

        add_timer(200,
                  [this]()
                  {
                      if (m_password_entry)
                          m_password_entry->set_focus(true);
                  });
    }

    void on_unlock()
    {
        std::string password = m_password_entry->text();
        struct passwd *pw = getpwuid(getuid());
        std::string username = pw ? pw->pw_name : "";

        if (validate_password(username, password))
        {
            LOG_INFO << "Unlock successful.";
            g_unlocked = true;
            quit();
        }
        else
        {
            m_error_label->set_text(i18n().tr("core.polkit.auth_failed"));
            m_password_entry->set_text("");
            m_password_entry->set_focus(true);
        }
    }

    TextBox<PasswordPolicy> *m_password_entry{nullptr};
    Label *m_error_label{nullptr};
};

int main(int argc, char **argv)
{
    Logger::instance().init("horizon-lock");

    i18n().add_search_path("../../../");
    i18n().add_search_path("../../");
    i18n().add_search_path("./");
    i18n().load_core_locales();

    LockWindow main_window(0);
    int monitor_count = main_window.get_monitor_count();
    LOG_INFO << "Starting Horizon Lock on " << monitor_count << " monitors.";

    std::vector<std::thread> secondary_threads;
    for (int i = 1; i < monitor_count; ++i)
    {
        secondary_threads.emplace_back([i]() {
            LockWindow secondary(i);
            secondary.run();
        });
    }

    main_window.run();

    for (auto &t : secondary_threads)
    {
        if (t.joinable())
            t.join();
    }

    return 0;
}
