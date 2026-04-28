#include <dbus/dbus.h>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <fstream>
#include <string>
#include <thread>
#include <pwd.h>
#include <sys/wait.h>
#include <horizon/I18n.hpp>

#include "AuthDialog.hpp"

using namespace horizon;
using namespace horizon::dbusutils;
using namespace horizon::polkit;

class PolkitAgentApp;
PolkitAgentApp* g_app = nullptr;

class PolkitAgentApp : public Application
{
public:
    PolkitAgentApp(int argc, char** argv) 
        : Application("horizon.polkit.agent", 0, 0, true, true) 
    {
        g_app = this;
        about_manager().set_app_title("Horizon Polkit Agent");
        about_manager().set_app_description("Horizon Authentication Agent");
        about_manager().set_app_version("0.1.0");
        about_manager().set_app_icon("dialog-password");
    }

    void handle_begin_auth(DBusConnection* conn, DBusMessage* pending_msg, 
                           const std::string& action_id, const std::string& message, 
                           const std::string& user, uint32_t uid, const std::string& cookie)
    {
        dbus_message_ref(pending_msg);

        std::thread([conn, pending_msg, user, uid, cookie]() {
            auto dialog = std::make_unique<AuthDialog>("", "", user);
            bool success_achieved = false;

            dialog->when_authenticated.connect([&](AuthSuccessEvent& ev) { 
                std::cout << "[Horizon Polkit Agent] Validando clave para " << user << "..." << std::endl;
                
                int pipe_fd[2];
                if (pipe(pipe_fd) == 0) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        close(pipe_fd[1]); dup2(pipe_fd[0], STDIN_FILENO); close(pipe_fd[0]);
                        const char* h = "/usr/lib/polkit-1/polkit-agent-helper-1";
                        if (access(h, X_OK) != 0) h = "/usr/libexec/polkit-agent-helper-1";
                        if (access(h, X_OK) != 0) h = "/usr/lib/policykit-1/polkit-agent-helper-1";
                        execl(h, h, user.c_str(), cookie.c_str(), nullptr);
                        exit(1);
                    } else if (pid > 0) {
                        close(pipe_fd[0]);
                        write(pipe_fd[1], ev.password.c_str(), ev.password.length());
                        write(pipe_fd[1], "\n", 1);
                        close(pipe_fd[1]);
                        
                        int status; waitpid(pid, &status, 0);
                        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                            std::cout << "[Horizon Polkit Agent] ¡Clave correcta!" << std::endl;
                            
                            // Avisar a la Autoridad
                            DBusMessage* call = dbus_message_new_method_call("org.freedesktop.PolicyKit1", "/org/freedesktop/PolicyKit1/Authority", "org.freedesktop.PolicyKit1.Authority", "AuthenticationAgentResponse");
                            if (call) {
                                DBusMessageIter iter, sub_iter, dict_iter, entry_iter, var_iter;
                                dbus_message_iter_init_append(call, &iter);
                                const char* cookie_ptr = cookie.c_str();
                                dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &cookie_ptr);
                                dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, nullptr, &sub_iter);
                                const char* kind = "unix-user"; dbus_message_iter_append_basic(&sub_iter, DBUS_TYPE_STRING, &kind);
                                dbus_message_iter_open_container(&sub_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
                                dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
                                const char* uid_k = "uid"; dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &uid_k);
                                dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "u", &var_iter);
                                dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_UINT32, &uid);
                                dbus_message_iter_close_container(&entry_iter, &var_iter);
                                dbus_message_iter_close_container(&dict_iter, &entry_iter);
                                dbus_message_iter_close_container(&sub_iter, &dict_iter);
                                dbus_message_iter_close_container(&iter, &sub_iter);

                                dbus_connection_send(conn, call, nullptr);
                                dbus_message_unref(call);
                            }
                            success_achieved = true;
                            dialog->quit(); // Cerrar ventana solo si hay exito
                        } else {
                            std::cerr << "[Horizon Polkit Agent] Clave incorrecta." << std::endl;
                            dialog->show_error(i18n().tr("core.polkit.auth_failed"));
                        }
                    }
                }
            });

            dialog->initialize();
            dialog->run(); // Se bloquea aqui hasta que success_achieved sea true o cierren la ventana

            // Siempre respondemos al mensaje original de D-Bus al final
            DBusMessage* reply = dbus_message_new_method_return(pending_msg);
            if (reply) {
                dbus_connection_send(conn, reply, nullptr);
                dbus_message_unref(reply);
            }
            dbus_message_unref(pending_msg);
            std::cout << "[Horizon Polkit Agent] Ciclo finalizado." << std::endl;
        }).detach();
    }
};

std::string get_username_from_uid(uint32_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? std::string(pw->pw_name) : std::to_string(uid);
}

DBusHandlerResult agent_message_handler(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    if (dbus_message_is_method_call(msg, "org.freedesktop.PolicyKit1.AuthenticationAgent", "BeginAuthentication"))
    {
        DBusMessageIter iter; dbus_message_iter_init(msg, &iter);
        const char *action_id, *message, *icon_name, *cookie_raw;
        dbus_message_iter_get_basic(&iter, &action_id); dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &message); dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &icon_name); dbus_message_iter_next(&iter);
        dbus_message_iter_next(&iter); // details
        dbus_message_iter_get_basic(&iter, &cookie_raw); dbus_message_iter_next(&iter);
        
        std::string cookie = cookie_raw;
        std::string t_name = "root"; uint32_t t_uid = 0; bool found = false;
        DBusMessageIter array_iter; dbus_message_iter_recurse(&iter, &array_iter);
        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
            DBusMessageIter struct_iter, dict_iter, entry_iter, var_iter;
            dbus_message_iter_recurse(&array_iter, &struct_iter);
            const char* kind; dbus_message_iter_get_basic(&struct_iter, &kind);
            if (std::string(kind) == "unix-user") {
                dbus_message_iter_next(&struct_iter); dbus_message_iter_recurse(&struct_iter, &dict_iter);
                while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
                    dbus_message_iter_recurse(&dict_iter, &entry_iter);
                    const char* key; dbus_message_iter_get_basic(&entry_iter, &key);
                    if (std::string(key) == "uid") {
                        dbus_message_iter_next(&entry_iter); dbus_message_iter_recurse(&entry_iter, &var_iter);
                        dbus_message_iter_get_basic(&var_iter, &t_uid);
                        t_name = get_username_from_uid(t_uid); found = true; break;
                    }
                    dbus_message_iter_next(&dict_iter);
                }
            }
            if (found) break;
            dbus_message_iter_next(&array_iter);
        }
        if (g_app) g_app->handle_begin_auth(conn, msg, action_id, message, t_name, t_uid, cookie);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int main(int argc, char** argv)
{
    dbus_threads_init_default();

    I18n::add_search_path("../../../share/locales");
    I18n::add_search_path("../../share/locales");
    I18n::add_search_path("../share/locales");
    I18n::add_search_path("../../../share");
    I18n::add_search_path("../../share");
    I18n::add_search_path("../share");

    horizon::Logger::instance().init("horizon-polkit-agent");
    PolkitAgentApp app(argc, argv);
    std::thread dbus_thread([]() {
        DBusError error; dbus_error_init(&error);
        DBusConnection* conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
        if (!conn) return;
        
        DBusObjectPathVTable vtable = { nullptr, agent_message_handler, nullptr, nullptr, nullptr, nullptr };
        dbus_connection_register_object_path(conn, "/org/horizon/PolkitAgent", &vtable, nullptr);
        
        DBusMessage* msg = dbus_message_new_method_call("org.freedesktop.PolicyKit1", "/org/freedesktop/PolicyKit1/Authority", "org.freedesktop.PolicyKit1.Authority", "RegisterAuthenticationAgent");
        DBusMessageIter iter, sub_iter, dict_iter, entry_iter, var_iter;
        dbus_message_iter_init_append(msg, &iter);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_STRUCT, nullptr, &sub_iter);
        const char* sid_v = getenv("XDG_SESSION_ID");
        if (sid_v) {
            const char* kind = "unix-session"; dbus_message_iter_append_basic(&sub_iter, DBUS_TYPE_STRING, &kind);
            dbus_message_iter_open_container(&sub_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
            dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
            const char* sid_k = "session-id"; dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &sid_k);
            dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "s", &var_iter);
            dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_STRING, &sid_v);
            dbus_message_iter_close_container(&entry_iter, &var_iter); dbus_message_iter_close_container(&dict_iter, &entry_iter);
        } else {
            const char* kind = "unix-process"; dbus_message_iter_append_basic(&sub_iter, DBUS_TYPE_STRING, &kind);
            dbus_message_iter_open_container(&sub_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
            uint32_t pid = getpid(); uint32_t uid = getuid(); 
            std::ifstream stat_file("/proc/self/stat"); std::string tmp; for (int i = 0; i < 21; ++i) stat_file >> tmp;
            uint64_t st = 0; stat_file >> st;
            auto add_p = [&](const char* k, int t, const void* v, const char* s) {
                dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
                dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &k);
                dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, s, &var_iter);
                dbus_message_iter_append_basic(&var_iter, t, v);
                dbus_message_iter_close_container(&entry_iter, &var_iter); dbus_message_iter_close_container(&dict_iter, &entry_iter);
            };
            add_p("pid", DBUS_TYPE_UINT32, &pid, "u"); add_p("uid", DBUS_TYPE_UINT32, &uid, "u"); add_p("start-time", DBUS_TYPE_UINT64, &st, "t");
        }
        dbus_message_iter_close_container(&sub_iter, &dict_iter); dbus_message_iter_close_container(&iter, &sub_iter);
        const char* loc = getenv("LANG"); if(!loc) loc = "es_AR.UTF-8";
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &loc);
        const char* path = "/org/horizon/PolkitAgent"; dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &path);
        
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, 5000, &error);
        if (reply) { std::cout << "[Horizon Polkit Agent] ¡REGISTRO EXITOSO!" << std::endl; dbus_message_unref(reply); }
        dbus_message_unref(msg);
        
        while (dbus_connection_read_write_dispatch(conn, 100)) {}
    });
    dbus_thread.join();
    return 0;
}
