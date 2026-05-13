#include "horizon/storage/RemoteManager.hpp"
#include <gio/gio.h>
#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif
#include <horizon/Logger.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <thread>
#include <horizon/I18n.hpp>

namespace horizon::storage
{
    struct RemoteManager::Private {};

    RemoteManager::RemoteManager() : d(std::make_unique<Private>()) {}
    RemoteManager::~RemoteManager() = default;

    // Helper: find FUSE mount path for a URI by scanning /proc/mounts
    static std::string find_fuse_path_for_uri(const std::string &uri)
    {
        // After a successful gio mount, GVFS mounts under ~/.gvfs or /run/user/*/gvfs
        std::vector<std::string> search_roots;
        const char *xdg_runtime = getenv("XDG_RUNTIME_DIR");
        if (xdg_runtime) search_roots.push_back(std::string(xdg_runtime) + "/gvfs");
        const char *home = getenv("HOME");
        if (home) search_roots.push_back(std::string(home) + "/.gvfs");
        search_roots.push_back("/run/user/1000/gvfs");

        for (const auto &root : search_roots)
        {
            if (!std::filesystem::exists(root)) continue;
            for (const auto &entry : std::filesystem::directory_iterator(root))
            {
                // GVFS names dirs like "ftp:host=test.rebex.net" or similar
                std::string dirname = entry.path().filename().string();
                // Extract protocol and host from uri to match
                std::string lower_uri = uri;
                std::transform(lower_uri.begin(), lower_uri.end(), lower_uri.begin(), ::tolower);
                // Remove protocol prefix
                auto pos = lower_uri.find("://");
                std::string host_part = (pos != std::string::npos) ? lower_uri.substr(pos + 3) : lower_uri;
                // Remove trailing path/slash
                auto slash = host_part.find('/');
                if (slash != std::string::npos) host_part = host_part.substr(0, slash);

                std::string lower_dirname = dirname;
                std::transform(lower_dirname.begin(), lower_dirname.end(), lower_dirname.begin(), ::tolower);
                if (lower_dirname.find(host_part) != std::string::npos)
                {
                    return entry.path().string();
                }
            }
        }
        return "";
    }

    void RemoteManager::when_mount(const std::string &uri,
                                   const RemoteCredentials &credentials,
                                   std::function<void(RemoteMountResult)> callback)
    {
        LOG_INFO << "RemoteManager: Intentando montar " << uri;

        // Run gio mount in a background thread. This subprocess has its own GLib event
        // loop so GVFS communication works correctly, bypassing Horizon's Wayland loop.
        std::thread([uri, credentials, callback]() {
            // Build the command. For anonymous/guest, just run gio mount <uri>.
            // For authenticated, use environment variables that gio mount can pick up,
            // or pass credentials via a wrapper script.
            std::string cmd;

            if (credentials.is_guest || credentials.username.empty())
            {
                cmd = "gio mount \"" + uri + "\" < /dev/null 2>&1";
            }
            else
            {
                std::string auth_uri = uri;
                auto proto_end = uri.find("://");
                if (proto_end != std::string::npos)
                {
                    std::string proto = uri.substr(0, proto_end + 3);
                    std::string rest = uri.substr(proto_end + 3);
                    auto at_pos = rest.find('@');
                    if (at_pos != std::string::npos) rest = rest.substr(at_pos + 1);
                    auth_uri = proto + credentials.username + "@" + rest; // Username in URI
                }
                
                // Escape password for shell (basic mitigation for simple characters, though full escaping is complex)
                // For safety in popen, we use single quotes and replace single quotes with '"'"'
                std::string safe_pass = credentials.password;
                size_t pos = 0;
                while ((pos = safe_pass.find("'", pos)) != std::string::npos) {
                    safe_pass.replace(pos, 1, "'\"'\"'");
                    pos += 5;
                }
                
                // SMB often asks for Domain first, then Password. 
                // FTP/SFTP just asks for Password.
                if (uri.find("smb://") == 0) {
                    // Send newline for Domain, then password
                    cmd = "printf '\\n%s\\n' '" + safe_pass + "' | gio mount \"" + auth_uri + "\" 2>&1";
                } else {
                    // Just password
                    cmd = "printf '%s\\n' '" + safe_pass + "' | gio mount \"" + auth_uri + "\" 2>&1";
                }
            }

            LOG_INFO << "RemoteManager: Ejecutando gio mount...";

            FILE *pipe = popen(cmd.c_str(), "r");
            RemoteMountResult result;

            if (!pipe)
            {
                result.success = false;
                result.message = "No se pudo ejecutar gio mount";
                LOG_ERROR << "RemoteManager: popen failed";
                callback(result);
                return;
            }

            std::string output;
            char buf[256];
            while (fgets(buf, sizeof(buf), pipe)) output += buf;
            int exit_code = pclose(pipe);

            // Trim output
            while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
                output.pop_back();

            LOG_INFO << "RemoteManager: gio mount exit=" << exit_code << " output=[" << output << "]";

            if (exit_code == 0)
            {
                result.success = true;
                result.message = "Mounted successfully";
                result.mount_path = find_fuse_path_for_uri(uri);
                LOG_INFO << "RemoteManager: Montaje exitoso. FUSE path=" << result.mount_path;
            }
            else
            {
                result.success = false;
                result.message = output.empty() ? "Error desconocido al montar" : output;
                LOG_ERROR << "RemoteManager: Error de montaje: " << result.message;
            }

            callback(result);
        }).detach();
    }

    void RemoteManager::when_unmount(const std::string &mount_path, std::function<void(bool, std::string)> callback)
    {
        std::thread([mount_path, callback]() {
            std::string cmd = "gio mount -u \"" + mount_path + "\" < /dev/null 2>&1";
            LOG_INFO << "RemoteManager: Desmontando path: " << mount_path;
            FILE *pipe = popen(cmd.c_str(), "r");
            if (!pipe) { callback(false, "No se pudo ejecutar gio mount -u"); return; }
            std::string output;
            char buf[256];
            while (fgets(buf, sizeof(buf), pipe)) output += buf;
            int exit_code = pclose(pipe);
            while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) output.pop_back();
            LOG_INFO << "RemoteManager: gio mount -u exit=" << exit_code << " output=[" << output << "]";
            callback(exit_code == 0, exit_code == 0 ? "Desmontado con éxito" : output);
        }).detach();
    }

    void RemoteManager::when_unmount_by_uri(const std::string &uri, std::function<void(bool, std::string)> callback)
    {
        std::thread([uri, callback]() {
            std::string cmd = "gio mount -u \"" + uri + "\" < /dev/null 2>&1";
            LOG_INFO << "RemoteManager: Desmontando URI: " << uri;
            FILE *pipe = popen(cmd.c_str(), "r");
            if (!pipe) { callback(false, "No se pudo ejecutar gio mount -u"); return; }
            std::string output;
            char buf[256];
            while (fgets(buf, sizeof(buf), pipe)) output += buf;
            int exit_code = pclose(pipe);
            while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) output.pop_back();
            LOG_INFO << "RemoteManager: gio mount -u exit=" << exit_code << " output=[" << output << "]";
            callback(exit_code == 0, exit_code == 0 ? "Desmontado con éxito" : output);
        }).detach();
    }

    std::vector<RemoteMountInfo> RemoteManager::get_active_mounts()
    {
        std::vector<RemoteMountInfo> mounts;
        GVolumeMonitor *monitor = g_volume_monitor_get();
        GList *g_mounts = g_volume_monitor_get_mounts(monitor);

        for (GList *l = g_mounts; l != NULL; l = l->next)
        {
            GMount *mount = G_MOUNT(l->data);
            char *name = g_mount_get_name(mount);
            GFile *root = g_mount_get_root(mount);
            char *uri = g_file_get_uri(root);
            char *path = g_file_get_path(root);

            if (uri && (g_str_has_prefix(uri, "smb://") || g_str_has_prefix(uri, "ftp://") ||
                        g_str_has_prefix(uri, "sftp://") || g_str_has_prefix(uri, "dav://")))
            {
                RemoteMountInfo info;
                info.name = name;
                info.uri = uri;
                info.mount_path = path ? path : "";

                GIcon *icon = g_mount_get_icon(mount);
                if (icon)
                {
                    char *icon_str = g_icon_to_string(icon);
                    info.icon_name = icon_str;
                    g_free(icon_str);
                    g_object_unref(icon);
                }

                mounts.push_back(info);
            }

            g_free(name);
            g_free(uri);
            if (path) g_free(path);
            g_object_unref(root);
        }

        g_list_free_full(g_mounts, g_object_unref);
        g_object_unref(monitor);
        return mounts;
    }

#ifdef HAVE_LIBSECRET
    static const SecretSchema *get_remote_storage_schema()
    {
        static const SecretSchema schema = {
            "org.horizon.RemoteStorage",
            SECRET_SCHEMA_NONE,
            {
                {"uri", SECRET_SCHEMA_ATTRIBUTE_STRING},
                {"NULL", (SecretSchemaAttributeType)0},
            }};
        return &schema;
    }
#endif

    static std::string obfuscate(const std::string &s)
    {
        std::string out = s;
        for (size_t i = 0; i < out.size(); ++i)
            out[i] ^= 0x55;
        return out;
    }

    void RemoteManager::save_credentials(const std::string &uri, const RemoteCredentials &creds)
    {
#ifdef HAVE_LIBSECRET
        GError *error = NULL;
        std::string secret_value = creds.username + "|" + creds.password;

        secret_password_store_sync(get_remote_storage_schema(),
                                   SECRET_COLLECTION_DEFAULT,
                                   (i18n().tr("core.storage.mount_dialog.title") + " (" + uri + ")").c_str(),
                                   secret_value.c_str(),
                                   NULL, &error,
                                   "uri", uri.c_str(),
                                   NULL);

        if (error)
        {
            LOG_ERROR << "RemoteManager: Failed to save credentials to keyring: " << error->message;
            g_error_free(error);
        }
        else
        {
            LOG_INFO << "RemoteManager: Credentials saved securely to keyring for " << uri;
            return;
        }
#endif
        LOG_WARNING << "RemoteManager: libsecret not found or failed. Falling back to insecure JSON storage.";

        std::string home = getenv("HOME") ? getenv("HOME") : "/tmp";
        std::string dir = home + "/.config/horizon";
        std::string path = dir + "/remote_credentials.json";

        if (!std::filesystem::exists(dir))
            std::filesystem::create_directories(dir);

        nlohmann::json j;
        if (std::filesystem::exists(path))
        {
            try { std::ifstream f(path); f >> j; } catch (...) {}
        }

        if (!j.is_object()) j = nlohmann::json::object();

        j[uri] = {{"username", creds.username}, {"password", obfuscate(creds.password)},
                  {"is_guest", creds.is_guest}, {"insecure", true}};

        try { std::ofstream f(path); f << j.dump(4); }
        catch (...) { LOG_ERROR << "RemoteManager: Failed to save credentials to JSON for " << uri; }
    }

    bool RemoteManager::get_credentials(const std::string &uri, RemoteCredentials &out_creds)
    {
#ifdef HAVE_LIBSECRET
        GError *error = NULL;
        char *password = secret_password_lookup_sync(get_remote_storage_schema(),
                                                     NULL, &error,
                                                     "uri", uri.c_str(), NULL);
        if (password)
        {
            std::string full_val = password;
            size_t pos = full_val.find('|');
            if (pos != std::string::npos)
            {
                out_creds.username = full_val.substr(0, pos);
                out_creds.password = full_val.substr(pos + 1);
            }
            else { out_creds.password = full_val; }
            out_creds.remember = true;
            out_creds.is_guest = false;
            secret_password_free(password);
            return true;
        }
        if (error) { LOG_ERROR << "RemoteManager: lookup error: " << error->message; g_error_free(error); }
#endif

        std::string home = getenv("HOME") ? getenv("HOME") : "/tmp";
        std::string path = home + "/.config/horizon/remote_credentials.json";
        if (!std::filesystem::exists(path)) return false;

        try
        {
            std::ifstream f(path);
            nlohmann::json j;
            f >> j;
            if (j.contains(uri))
            {
                auto entry = j[uri];
                out_creds.username = entry.value("username", "");
                out_creds.password = obfuscate(entry.value("password", ""));
                out_creds.is_guest = entry.value("is_guest", false);
                out_creds.remember = true;
                return true;
            }
        }
        catch (...) {}

        return false;
    }
} // namespace horizon::storage
