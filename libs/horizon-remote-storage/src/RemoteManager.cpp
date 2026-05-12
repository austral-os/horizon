#include "horizon-remote-storage/RemoteManager.hpp"
#include <gio/gio.h>
#include <horizon/Logger.hpp>
#include <thread>

namespace horizon::storage
{
    struct RemoteManager::Private {};

    RemoteManager::RemoteManager() : d(std::make_unique<Private>()) {}
    RemoteManager::~RemoteManager() = default;

    struct MountContext {
        std::function<void(RemoteMountResult)> callback;
        GFile* file;
        GMountOperation* op;
    };

    static void on_mount_finished(GObject* source_object, GAsyncResult* res, gpointer user_data)
    {
        auto* context = static_cast<MountContext*>(user_data);
        GError* error = nullptr;
        
        g_file_mount_enclosing_volume_finish(G_FILE(source_object), res, &error);

        RemoteMountResult result;
        if (error) {
            result.success = false;
            result.message = error->message;
            g_error_free(error);
        } else {
            result.success = true;
            result.message = "Mounted successfully";
            
            GMount* mount = g_file_find_enclosing_mount(context->file, nullptr, nullptr);
            if (mount) {
                GFile* root = g_mount_get_root(mount);
                char* path = g_file_get_path(root);
                if (path) {
                    result.mount_path = path;
                    g_free(path);
                }
                g_object_unref(root);
                g_object_unref(mount);
            }
        }

        if (context->callback) context->callback(result);
        g_object_unref(context->file);
        g_object_unref(context->op);
        delete context;
    }

    void RemoteManager::mount(const std::string& uri, 
                               const RemoteCredentials& credentials,
                               std::function<void(RemoteMountResult)> callback)
    {
        std::thread([uri, credentials, callback]() {
            LOG_INFO << "RemoteManager: Intentando montar URI: " << uri;
            GMainContext* context = g_main_context_new();
            g_main_context_push_thread_default(context);

            GFile* file = g_file_new_for_uri(uri.c_str());
            if (!file) {
                LOG_ERROR << "RemoteManager: No se pudo crear GFile para URI: " << uri;
                if (callback) callback({false, "Invalid URI"});
                return;
            }
            GMountOperation* op = g_mount_operation_new();

            // Connect to signals to avoid hanging if GIO asks for something
            g_signal_connect(op, "ask-password", G_CALLBACK(+[](GMountOperation* op, const char* message, const char* default_user, const char* default_domain, GAskPasswordFlags flags, gpointer user_data) {
                LOG_INFO << "RemoteManager: GIO solicita contraseña: " << message;
                // We don't want to handle interactive password asking here, 
                // we want it to fail so we can show our own dialog in ArkFM.
                g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
            }), nullptr);

            g_signal_connect(op, "ask-question", G_CALLBACK(+[](GMountOperation* op, const char* message, const char** choices, gpointer user_data) {
                LOG_INFO << "RemoteManager: GIO solicita respuesta a pregunta: " << message;
                g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
            }), nullptr);

            if (credentials.is_guest) {
                g_mount_operation_set_anonymous(op, TRUE);
            } else {
                if (!credentials.username.empty())
                    g_mount_operation_set_username(op, credentials.username.c_str());
                if (!credentials.password.empty())
                    g_mount_operation_set_password(op, credentials.password.c_str());
            }

            struct State {
                bool finished = false;
                RemoteMountResult result;
                GFile* file;
            } state;
            state.file = file;

            g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, nullptr, 
                [](GObject* source, GAsyncResult* res, gpointer user_data) {
                    auto* s = static_cast<State*>(user_data);
                    GError* error = nullptr;
                    g_file_mount_enclosing_volume_finish(G_FILE(source), res, &error);

                    if (error) {
                        s->result.success = false;
                        s->result.message = error->message;
                        LOG_ERROR << "RemoteManager: Error de GIO: " << error->message;
                        g_error_free(error);
                    } else {
                        s->result.success = true;
                        s->result.message = "Mounted successfully";
                        
                        GMount* mount = g_file_find_enclosing_mount(s->file, nullptr, nullptr);
                        if (mount) {
                            GFile* root = g_mount_get_root(mount);
                            char* path = g_file_get_path(root);
                            if (path) {
                                s->result.mount_path = path;
                                LOG_INFO << "RemoteManager: Recurso montado en " << path;
                                g_free(path);
                            }
                            g_object_unref(root);
                            g_object_unref(mount);
                        }
                    }
                    s->finished = true;
                }, &state);

            while (!state.finished) {
                g_main_context_iteration(context, TRUE);
            }

            if (callback) callback(state.result);

            g_object_unref(file);
            g_object_unref(op);
            g_main_context_pop_thread_default(context);
            g_main_context_unref(context);
        }).detach();
    }

    void RemoteManager::unmount(const std::string& mount_path, std::function<void(bool, std::string)> callback)
    {
        std::thread([mount_path, callback]() {
            GMainContext* context = g_main_context_new();
            g_main_context_push_thread_default(context);

            GFile* file = g_file_new_for_path(mount_path.c_str());
            GMount* mount = g_file_find_enclosing_mount(file, nullptr, nullptr);
            
            if (!mount) {
                if (callback) callback(false, "No mount found at this path");
                g_object_unref(file);
                g_main_context_pop_thread_default(context);
                g_main_context_unref(context);
                return;
            }

            struct State {
                bool finished = false;
                bool success = false;
                std::string message;
            } state;

            g_mount_unmount_with_operation(mount, G_MOUNT_UNMOUNT_NONE, nullptr, nullptr, 
                [](GObject* src, GAsyncResult* res, gpointer user_data) {
                    auto* s = static_cast<State*>(user_data);
                    GError* error = nullptr;
                    s->success = g_mount_unmount_with_operation_finish(G_MOUNT(src), res, &error);
                    if (error) {
                        s->message = error->message;
                        g_error_free(error);
                    }
                    s->finished = true;
                }, &state);

            while (!state.finished) {
                g_main_context_iteration(context, TRUE);
            }

            if (callback) callback(state.success, state.message);

            g_object_unref(mount);
            g_object_unref(file);
            g_main_context_pop_thread_default(context);
            g_main_context_unref(context);
        }).detach();
    }

    std::vector<RemoteMountInfo> RemoteManager::get_active_mounts()
    {
        std::vector<RemoteMountInfo> mounts;
        GVolumeMonitor* monitor = g_volume_monitor_get();
        GList* g_mounts = g_volume_monitor_get_mounts(monitor);

        for (GList* l = g_mounts; l != nullptr; l = l->next) {
            GMount* mount = G_MOUNT(l->data);
            char* name = g_mount_get_name(mount);
            GFile* root = g_mount_get_root(mount);
            char* uri = g_file_get_uri(root);
            char* path = g_file_get_path(root);
            
            GIcon* icon = g_mount_get_icon(mount);
            std::string icon_name = "folder-remote";
            if (G_IS_THEMED_ICON(icon)) {
                const char* const* names = g_themed_icon_get_names(G_THEMED_ICON(icon));
                if (names && names[0]) icon_name = names[0];
            }

            std::string s_uri = uri ? uri : "";
            if (s_uri.find("://") != std::string::npos && s_uri.find("file://") != 0) {
                mounts.push_back({name ? name : "Remote Resource", s_uri, path ? path : "", icon_name});
            }

            if (name) g_free(name);
            if (uri) g_free(uri);
            if (path) g_free(path);
            if (root) g_object_unref(root);
            if (icon) g_object_unref(icon);
        }

        g_list_free_full(g_mounts, g_object_unref);
        g_object_unref(monitor);
        return mounts;
    }
}
