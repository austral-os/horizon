#include <horizon/apt/AppStoreClient.hpp>
#include <libsoup/soup.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace horizon::apt {

struct AppStoreClient::Private {
    SoupSession* session;
    std::string base_url = "https://appstore-api.hdrdevs.com.ar/api";

    std::string get_cache_dir() const {
        const char* home = getenv("HOME");
        return home ? std::string(home) + "/.cache/horizon-appstore" : "/tmp/horizon-appstore-cache";
    }

    std::string get_cache_file_path(const std::string& url) const {
        std::string safe_name = url;
        for (char& c : safe_name) {
            if (!isalnum(c)) c = '_';
        }
        return get_cache_dir() + "/api/" + safe_name + ".json";
    }

    bool is_cache_valid(const std::string& filepath) const {
        if (!std::filesystem::exists(filepath)) return false;
        
        auto ftime = std::filesystem::last_write_time(filepath);
        auto now = std::filesystem::file_time_type::clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::hours>(now - ftime).count();
        
        return diff < 24; // 24 hours TTL
    }

    std::string get_config_url() const {
        const char* home = getenv("HOME");
        if (!home) return "";
        std::string path = std::string(home) + "/.config/horizon/appstore.json";
        try {
            if (!std::filesystem::exists(path)) return "";
            std::ifstream in(path);
            json j;
            in >> j;
            if (j.contains("api_url") && !j["api_url"].is_null()) {
                std::string url = j["api_url"].get<std::string>();
                if (!url.empty() && url.back() == '/') {
                    url.pop_back();
                }
                return url;
            }
        } catch (...) {}
        return "";
    }

    std::optional<std::string> perform_http_get_raw(const std::string& url) const {
        SoupSession* session = soup_session_new();
        SoupMessage* msg = soup_message_new("GET", url.c_str());
        if (!msg) {
            g_object_unref(session);
            return std::nullopt;
        }

        GError* error = nullptr;
        GBytes* bytes = soup_session_send_and_read(session, msg, nullptr, &error);
        if (error) {
            std::cerr << "Request failed for " << url << ": " << error->message << std::endl;
            g_error_free(error);
            g_object_unref(msg);
            g_object_unref(session);
            return std::nullopt;
        }

        gsize size;
        const char* data = static_cast<const char*>(g_bytes_get_data(bytes, &size));
        std::string ret(data, size);
        g_bytes_unref(bytes);
        g_object_unref(msg);
        g_object_unref(session);
        return ret;
    }

    std::optional<json> perform_http_get(const std::string& url) const {
        auto raw = perform_http_get_raw(url);
        if (!raw) return std::nullopt;
        try {
            return json::parse(*raw);
        } catch (...) {
            return std::nullopt;
        }
    }

    std::optional<json> perform_http_post(const std::string& url, const json& body) const {
        std::string body_str = body.dump();
        SoupSession* session = soup_session_new();
        SoupMessage* msg = soup_message_new("POST", url.c_str());
        if (!msg) {
            g_object_unref(session);
            return std::nullopt;
        }

        GBytes* body_bytes = g_bytes_new(body_str.c_str(), body_str.size());
        soup_message_set_request_body_from_bytes(msg, "application/json", body_bytes);
        g_bytes_unref(body_bytes);

        GError* error = nullptr;
        GBytes* bytes = soup_session_send_and_read(session, msg, nullptr, &error);
        
        if (error) {
            std::cerr << "POST failed for " << url << ": " << error->message << std::endl;
            g_error_free(error);
            g_object_unref(msg);
            g_object_unref(session);
            return std::nullopt;
        }

        gsize size;
        const char* data = static_cast<const char*>(g_bytes_get_data(bytes, &size));
        std::optional<json> ret = std::nullopt;
        try {
            ret = json::parse(std::string_view(data, size));
        } catch (...) {}
        g_bytes_unref(bytes);
        g_object_unref(msg);
        g_object_unref(session);
        return ret;
    }

    template<typename T>
    void make_async_request(const std::string& endpoint, std::function<void(std::optional<T>)> callback, std::function<std::optional<T>(const json&)> parser, bool use_cache = true) {
        std::string custom_url = get_config_url();
        std::string full_custom_url = custom_url.empty() ? "" : custom_url + endpoint;
        std::string full_default_url = base_url + endpoint;

        if (use_cache) {
            std::string cache_path = get_cache_file_path(full_custom_url.empty() ? full_default_url : full_custom_url);
            if (is_cache_valid(cache_path)) {
                std::thread([cache_path, callback = std::move(callback), parser = std::move(parser)]() {
                    try {
                        std::ifstream in(cache_path);
                        json j;
                        in >> j;
                        if (j.value("status", "") == "success") {
                            callback(parser(j["data"]));
                            return;
                        }
                    } catch (...) {}
                    callback(std::nullopt);
                }).detach();
                return;
            }
        }

        std::thread([full_custom_url, full_default_url, use_cache, cache_dir = get_cache_dir(), this, callback = std::move(callback), parser = std::move(parser)]() {
            std::optional<json> j_res = std::nullopt;
            std::string success_url = "";
            
            if (!full_custom_url.empty()) {
                j_res = perform_http_get(full_custom_url);
                if (j_res && j_res->value("status", "") == "success") {
                    success_url = full_custom_url;
                }
            }
            
            if (!j_res || j_res->value("status", "") != "success") {
                j_res = perform_http_get(full_default_url);
                if (j_res && j_res->value("status", "") == "success") {
                    success_url = full_default_url;
                }
            }

            if (j_res && j_res->value("status", "") == "success") {
                if (use_cache && !success_url.empty()) {
                    std::error_code ec;
                    std::filesystem::create_directories(cache_dir + "/api", ec);
                    std::ofstream out(get_cache_file_path(success_url));
                    if (out.is_open()) {
                        out << j_res->dump();
                        out.close();
                    }
                }
                callback(parser((*j_res)["data"]));
            } else {
                callback(std::nullopt);
            }
        }).detach();
    }

    void make_post_request(const std::string& endpoint, const json& body, std::function<void(bool)> callback) {
        std::string custom_url = get_config_url();
        std::string full_custom_url = custom_url.empty() ? "" : custom_url + endpoint;
        std::string full_default_url = base_url + endpoint;

        std::thread([full_custom_url, full_default_url, body, this, callback = std::move(callback)]() {
            std::optional<json> j_res = std::nullopt;
            
            if (!full_custom_url.empty()) {
                j_res = perform_http_post(full_custom_url, body);
            }
            
            if (!j_res || j_res->value("status", "") != "success") {
                j_res = perform_http_post(full_default_url, body);
            }

            if (j_res && j_res->value("status", "") == "success") {
                callback(true);
            } else {
                callback(false);
            }
        }).detach();
    }
};

AppStoreClient::AppStoreClient() : d(new Private) {
    d->session = soup_session_new();
}

AppStoreClient::~AppStoreClient() {
    if (d->session) {
        g_object_unref(d->session);
    }
    delete d;
}

// Helpers to parse JSON safely
static std::optional<std::string> get_opt_string(const json& j, const std::string& key) {
    if (j.contains(key) && !j[key].is_null()) {
        return j[key].get<std::string>();
    }
    return std::nullopt;
}

static std::optional<int> get_opt_int(const json& j, const std::string& key) {
    if (j.contains(key) && !j[key].is_null()) {
        return j[key].get<int>();
    }
    return std::nullopt;
}

void AppStoreClient::get_apps_async(std::optional<int> category_id, const std::string& sort_by, const std::string& order, int limit, int offset, const std::string& lang, std::function<void(std::optional<std::vector<AppInfo>>)> callback) {
    std::string endpoint = "/apps?sort_by=" + sort_by + "&order=" + order + "&limit=" + std::to_string(limit) + "&offset=" + std::to_string(offset) + "&lang=" + lang;
    if (category_id) {
        endpoint += "&category_id=" + std::to_string(*category_id);
    }

    d->make_async_request<std::vector<AppInfo>>(endpoint, std::move(callback), [](const json& data) -> std::optional<std::vector<AppInfo>> {
        std::vector<AppInfo> apps;
        if (!data.is_array()) return std::nullopt;
        for (const auto& item : data) {
            AppInfo app;
            app.id = item.value("id", 0);
            app.package_name = item.value("package_name", "");
            app.category_id = get_opt_int(item, "category_id");
            app.category_name = item.value("category_name", "");
            app.version_id = item.value("version_id", 0);
            app.version_string = item.value("version_string", "");
            app.icon_path = item.value("icon_path", "");
            app.published_at = item.value("published_at", "");
            app.name = item.value("name", "");
            app.avg_rating = item.value("avg_rating", 0.0);
            app.review_count = item.value("review_count", 0);
            app.extra_translations_count = item.value("extra_translations_count", 0);
            apps.push_back(app);
        }
        return apps;
    });
}

void AppStoreClient::get_featured_apps_async(const std::string& lang, std::function<void(std::optional<std::vector<FeaturedApp>>)> callback) {
    std::string endpoint = "/featured?lang=" + lang;
    d->make_async_request<std::vector<FeaturedApp>>(endpoint, std::move(callback), [](const json& data) -> std::optional<std::vector<FeaturedApp>> {
        std::vector<FeaturedApp> apps;
        if (!data.is_array()) return std::nullopt;
        for (const auto& item : data) {
            FeaturedApp app;
            app.order_index = item.value("order_index", 0);
            app.id = item.value("id", 0);
            app.package_name = item.value("package_name", "");
            app.category_name = item.value("category_name", "");
            app.version_string = item.value("version_string", "");
            app.icon_path = item.value("icon_path", "");
            app.name = item.value("name", "");
            app.description = item.value("description", "");
            app.avg_rating = item.value("avg_rating", 0.0);
            apps.push_back(app);
        }
        return apps;
    });
}

void AppStoreClient::get_app_details_async(const std::string& package_name, const std::string& lang, std::function<void(std::optional<AppDetails>)> callback) {
    std::string endpoint = "/apps/" + package_name + "?lang=" + lang;
    d->make_async_request<AppDetails>(endpoint, std::move(callback), [](const json& data) -> std::optional<AppDetails> {
        if (!data.contains("app") || !data.contains("versions")) return std::nullopt;
        
        AppDetails details;
        const auto& app = data["app"];
        details.id = app.value("id", 0);
        details.package_name = app.value("package_name", "");
        details.category_id = get_opt_int(app, "category_id");
        details.category_name = app.value("category_name", "");
        details.avg_rating = app.value("avg_rating", 0.0);
        
        for (const auto& v : data["versions"]) {
            AppVersionShort ver;
            ver.id = v.value("id", 0);
            ver.version_string = v.value("version_string", "");
            ver.published_at = v.value("published_at", "");
            ver.name = v.value("name", "");
            details.versions.push_back(ver);
        }
        return details;
    });
}

void AppStoreClient::get_app_version_details_async(const std::string& package_name, const std::string& version_string, const std::string& lang, std::function<void(std::optional<AppVersionDetails>)> callback) {
    std::string endpoint = "/apps/" + package_name + "/versions/" + version_string + "?lang=" + lang;
    d->make_async_request<AppVersionDetails>(endpoint, std::move(callback), [](const json& data) -> std::optional<AppVersionDetails> {
        AppVersionDetails details;
        details.id = data.value("id", 0);
        details.app_id = data.value("app_id", 0);
        details.version_string = data.value("version_string", "");
        details.published_at = data.value("published_at", "");
        details.icon_path = get_opt_string(data, "icon_path");
        details.source_url = get_opt_string(data, "source_url");
        details.signature = get_opt_string(data, "signature");
        details.name = data.value("name", "");
        details.description = get_opt_string(data, "description");
        details.release_notes = get_opt_string(data, "release_notes");
        
        if (data.contains("screenshots") && data["screenshots"].is_array()) {
            for (const auto& s : data["screenshots"]) {
                AppScreenshot shot;
                shot.id = s.value("id", 0);
                shot.image_path = s.value("image_path", "");
                details.screenshots.push_back(shot);
            }
        }
        return details;
    });
}

void AppStoreClient::get_app_reviews_async(const std::string& package_name, const std::string& version_string, std::function<void(std::optional<std::vector<Review>>)> callback) {
    std::string endpoint = "/apps/" + package_name + "/versions/" + version_string + "/reviews";
    d->make_async_request<std::vector<Review>>(endpoint, std::move(callback), [](const json& data) -> std::optional<std::vector<Review>> {
        std::vector<Review> reviews;
        if (!data.is_array()) return std::nullopt;
        for (const auto& item : data) {
            Review rev;
            rev.id = item.value("id", 0);
            rev.rating = item.value("rating", 0);
            rev.comment = item.value("comment", "");
            rev.created_at = item.value("created_at", "");
            reviews.push_back(rev);
        }
        return reviews;
    });
}

void AppStoreClient::post_app_review_async(const std::string& package_name, const std::string& version_string, int rating, const std::string& comment, std::function<void(bool)> callback) {
    std::string endpoint = "/apps/" + package_name + "/versions/" + version_string + "/reviews";
    json body = {
        {"rating", rating},
        {"comment", comment}
    };
    d->make_post_request(endpoint, body, std::move(callback));
}

void AppStoreClient::download_image_async(const std::string& image_path, std::function<void(std::optional<std::string>)> callback) {
    if (image_path.empty()) {
        callback(std::nullopt);
        return;
    }
    
    std::string custom_url = d->get_config_url();
    std::string full_custom_url = "";
    if (!custom_url.empty()) {
        auto pos = custom_url.find("/api");
        if (pos != std::string::npos) {
            full_custom_url = custom_url.substr(0, pos) + image_path;
        } else {
            full_custom_url = custom_url + image_path;
        }
    }
    
    std::string full_default_url = d->base_url.substr(0, d->base_url.find("/api")) + image_path;

    std::thread([full_custom_url, full_default_url, cache_dir = d->get_cache_dir(), image_path, this, callback = std::move(callback)]() {
        auto filename = std::filesystem::path(image_path).filename().string();
        std::string local_path = cache_dir + "/images/" + filename;
        
        if (std::filesystem::exists(local_path)) {
            auto ftime = std::filesystem::last_write_time(local_path);
            auto now = std::filesystem::file_time_type::clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::hours>(now - ftime).count();
            if (diff < 24) {
                callback(local_path);
                return;
            }
        }

        std::optional<std::string> raw = std::nullopt;
        if (!full_custom_url.empty()) {
            raw = d->perform_http_get_raw(full_custom_url);
        }
        
        if (!raw) {
            raw = d->perform_http_get_raw(full_default_url);
        }

        if (raw) {
            std::error_code ec;
            std::filesystem::create_directories(cache_dir + "/images/", ec);
            
            std::ofstream out(local_path, std::ios::binary);
            if (out.is_open()) {
                out.write(raw->data(), raw->size());
                out.close();
            }
            callback(local_path);
        } else {
            callback(std::nullopt);
        }
    }).detach();
}

void AppStoreClient::get_categories_async(std::function<void(std::optional<std::vector<Category>>)> callback) {
    std::string endpoint = "/categories";
    d->make_async_request<std::vector<Category>>(endpoint, std::move(callback), [](const json& data) -> std::optional<std::vector<Category>> {
        std::vector<Category> categories;
        if (!data.is_array()) return std::nullopt;
        for (const auto& item : data) {
            Category cat;
            cat.id = item.value("id", 0);
            cat.name = item.value("name", "");
            cat.icon = get_opt_string(item, "icon");
            categories.push_back(cat);
        }
        return categories;
    });
}

void AppStoreClient::clear_cache() {
    std::error_code ec;
    std::string cache_dir = d->get_cache_dir();
    if (std::filesystem::exists(cache_dir, ec)) {
        std::filesystem::remove_all(cache_dir, ec);
    }
}

} // namespace horizon::apt
