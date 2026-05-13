#include "horizon/storage/RemoteManager.hpp"
#include <gio/gio.h>
#include <horizon/Logger.hpp>
#include <thread>

namespace horizon::storage
{
    struct RemoteManager::Private {
        GVolumeMonitor* monitor;
        RemoteManager* parent;
        
        static void on_mount_changed(GVolumeMonitor*, GMount* mount, gpointer user_data) {
            auto* d = static_cast<RemoteManager::Private*>(user_data);
            LOG_INFO << "RemoteManager: Mount changed (added/removed)";
            RemoteStorageEventContext ctx;
            d->parent->when_changed.run(ctx);
        }
    };

    RemoteManager::RemoteManager() : d(std::make_unique<Private>()) {
        d->parent = this;
        d->monitor = g_volume_monitor_get();
        g_signal_connect(d->monitor, "mount-added", G_CALLBACK(Private::on_mount_changed), d.get());
        g_signal_connect(d->monitor, "mount-removed", G_CALLBACK(Private::on_mount_changed), d.get());
    }
    RemoteManager::~RemoteManager() {
        g_signal_handlers_disconnect_by_data(d->monitor, d.get());
        g_object_unref(d->monitor);
    }

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

    void RemoteManager::when_mount(const std::string& uri, 
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
            g_signal_connect(op, "ask-password", G_CALLBACK(+[](GMountOperation *op, const char *message, const char *default_user,
                               const char *default_domain, GAskPasswordFlags flags, gpointer user_data)
    {
        LOG_INFO << "RemoteManager: GIO solicita contraseña: " << message;
        
        const char* pwd = g_mount_operation_get_password(op);
        bool has_creds = (pwd && strlen(pwd) > 0) || g_mount_operation_get_anonymous(op);

        int attempts = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(op), "hzn-auth-attempts"));
        
        if (has_creds && attempts == 0) {
            LOG_INFO << "RemoteManager: Credenciales presentes, permitiendo intento a GIO.";
            g_object_set_data(G_OBJECT(op), "hzn-auth-attempts", GINT_TO_POINTER(1));
            g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
        } else {
            LOG_INFO << "RemoteManager: Abortando (sin credenciales o segundo intento).";
            g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);
        }
    }), nullptr);

            g_signal_connect(op, "ask-question", G_CALLBACK(+[](GMountOperation *op, const char *message, const char **choices,
                               gpointer user_data)
    {
        LOG_INFO << "RemoteManager: GIO solicita respuesta a pregunta: " << message;
        
        int attempts = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(op), "hzn-question-attempts"));
        
        if (attempts == 0) {
            g_object_set_data(G_OBJECT(op), "hzn-question-attempts", GINT_TO_POINTER(1));
            g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
        } else {
            g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);
        }
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
                std::string target_uri;
            } state;
            state.file = file;
            state.target_uri = uri;

            g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, nullptr, 
                [](GObject* source, GAsyncResult* res, gpointer user_data) {
                    auto* s = static_cast<State*>(user_data);
                    GError* error = nullptr;
                    g_file_mount_enclosing_volume_finish(G_FILE(source), res, &error);

                    if (error) {
                        if (error->domain == G_IO_ERROR && (error->code == G_IO_ERROR_ALREADY_MOUNTED)) {
                            LOG_INFO << "RemoteManager: GIO reporta que ya está montado. Buscando punto de montaje...";
                            s->result.success = true;
                            s->result.message = "Already mounted";
                        } else {
                            s->result.success = false;
                            s->result.message = error->message;
                            LOG_ERROR << "RemoteManager: Error de GIO (Domain: " << error->domain << " Code: " << error->code << "): " << error->message;
                        }
                        g_error_free(error);
                    } else {
                        s->result.success = true;
                        s->result.message = "Mounted successfully";
                    }

                    if (s->result.success) {
                        GMount* mount = g_file_find_enclosing_mount(s->file, nullptr, nullptr);
                        
                        // FALLBACK: If g_file_find_enclosing_mount fails, iterate all mounts
                        if (!mount) {
                            LOG_INFO << "RemoteManager: Fallback: Buscando montaje manualmente para URI: " << s->target_uri;
                            GVolumeMonitor* monitor = g_volume_monitor_get();
                            GList* mounts = g_volume_monitor_get_mounts(monitor);
                            GFile* target_file = g_file_new_for_uri(s->target_uri.c_str());
                            
                            for (GList* l = mounts; l != nullptr; l = l->next) {
                                GMount* m = G_MOUNT(l->data);
                                GFile* root = g_mount_get_root(m);
                                if (g_file_equal(root, target_file) || g_file_has_prefix(target_file, root)) {
                                    mount = G_MOUNT(g_object_ref(m));
                                    g_object_unref(root);
                                    break;
                                }
                                g_object_unref(root);
                            }
                            g_object_unref(target_file);
                            g_list_free_full(mounts, g_object_unref);
                            g_object_unref(monitor);
                        }

                        if (mount) {
                            GFile* root = g_mount_get_root(mount);
                            char* path = g_file_get_path(root);
                            if (path) {
                                s->result.mount_path = path;
                                LOG_INFO << "RemoteManager: Recurso montado en " << path;
                                g_free(path);
                            } else {
                                LOG_WARNING << "RemoteManager: No se pudo obtener la ruta local para el montaje.";
                            }
                            g_object_unref(root);
                            g_object_unref(mount);
                        } else {
                            LOG_WARNING << "RemoteManager: No se pudo encontrar el montaje que GIO dice que ya existe.";
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

    void RemoteManager::when_unmount(const std::string& mount_path, std::function<void(bool, std::string)> callback)
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

    void RemoteManager::when_unmount_by_uri(const std::string& uri, std::function<void(bool, std::string)> callback)
    {
        std::thread([uri, callback]() {
            GMainContext* context = g_main_context_new();
            g_main_context_push_thread_default(context);

            GFile* file = g_file_new_for_uri(uri.c_str());
            GMount* mount = g_file_find_enclosing_mount(file, nullptr, nullptr);
            
            if (!mount) {
                if (callback) callback(false, "No mount found for this URI");
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
        LOG_INFO << "RemoteManager: get_active_mounts() called";
        
        // Force GIO to process pending signals to get fresh data
        while (g_main_context_iteration(nullptr, FALSE));

        std::vector<RemoteMountInfo> mounts;
        GVolumeMonitor* monitor = d->monitor;
        GList* g_mounts = g_volume_monitor_get_mounts(monitor);
        
        LOG_INFO << "RemoteManager: GVolumeMonitor returned " << g_list_length(g_mounts) << " mounts";

        for (GList* l = g_mounts; l != nullptr; l = l->next) {
            GMount* mount = G_MOUNT(l->data);
            char* name = g_mount_get_name(mount);
            GFile* root = g_mount_get_root(mount);
            char* uri = g_file_get_uri(root);
            char* path = g_file_get_path(root);
            
            LOG_INFO << "RemoteManager: Found mount: " << (name ? name : "Unnamed") 
                     << " URI: " << (uri ? uri : "NULL") 
                     << " Path: " << (path ? path : "NULL");

            GIcon* icon = g_mount_get_icon(mount);
            std::string icon_name = "folder-remote";
            if (G_IS_THEMED_ICON(icon)) {
                const char* const* names = g_themed_icon_get_names(G_THEMED_ICON(icon));
                if (names && names[0]) icon_name = names[0];
            }

            std::string s_uri = uri ? uri : "";
            std::string s_path = path ? path : "";
            
            // LIBERAL FILTER:
            // 1. Include anything that doesn't start with file:// (standard remote URIs)
            // 2. Include anything that IS file:// but points to the GVFS FUSE mount point
            bool is_remote = !s_uri.empty() && s_uri.find("file://") != 0;
            
            if (!is_remote && s_path.find("/gvfs/") != std::string::npos) {
                is_remote = true;
            }
            
            // Exclude some internal URIs that we don't want to show
            if (s_uri.find("burn://") == 0 || s_uri.find("recent://") == 0 || s_uri.find("trash://") == 0) {
                is_remote = false;
            }

            if (is_remote) {
                LOG_INFO << "RemoteManager: Including as remote mount: " << s_uri << " (Path: " << s_path << ")";
                mounts.push_back({name ? name : "Remote Resource", s_uri, s_path, icon_name});
            } else {
                LOG_INFO << "RemoteManager: Excluding mount: " << s_uri;
            }

            if (name) g_free(name);
            if (uri) g_free(uri);
            if (path) g_free(path);
            if (root) g_object_unref(root);
            if (icon) g_object_unref(icon);
        }

        g_list_free_full(g_mounts, g_object_unref);
        return mounts;
    }
}
