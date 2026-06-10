#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace horizon::apt {

struct AppInfo {
    int id;
    std::string package_name;
    std::optional<int> category_id;
    std::string category_name;
    int version_id;
    std::string version_string;
    std::string icon_path;
    std::string published_at;
    std::string name;
    double avg_rating;
    int review_count;
    int extra_translations_count;
};

struct FeaturedApp {
    int order_index;
    int id;
    std::string package_name;
    std::string category_name;
    std::string version_string;
    std::string icon_path;
    std::string name;
    std::string description;
    double avg_rating = 0.0;
};

struct Category {
    int id;
    std::string name;
    std::optional<std::string> icon;
};

struct AppVersionShort {
    int id;
    std::string version_string;
    std::string published_at;
    std::string name;
};

struct AppDetails {
    int id;
    std::string package_name;
    std::optional<int> category_id;
    std::string category_name;
    double avg_rating = 0.0;
    std::vector<AppVersionShort> versions;
};

struct AppScreenshot {
    int id;
    std::string image_path;
};

struct AppVersionDetails {
    int id;
    int app_id;
    std::string version_string;
    std::string published_at;
    std::optional<std::string> icon_path;
    std::optional<std::string> source_url;
    std::optional<std::string> signature;
    std::string name;
    std::optional<std::string> description;
    std::optional<std::string> release_notes;
    std::vector<AppScreenshot> screenshots;
};

struct Review {
    int id;
    int rating;
    std::string comment;
    std::string created_at;
};

class AppStoreClient {
public:
    AppStoreClient();
    ~AppStoreClient();

    void get_apps_async(std::optional<int> category_id, 
                        const std::string& sort_by, 
                        const std::string& order, 
                        int limit, 
                        int offset, 
                        const std::string& lang, 
                        std::function<void(std::optional<std::vector<AppInfo>>)> callback);

    void get_featured_apps_async(const std::string& lang, 
                                 std::function<void(std::optional<std::vector<FeaturedApp>>)> callback);

    void get_app_details_async(const std::string& package_name, 
                               const std::string& lang, 
                               std::function<void(std::optional<AppDetails>)> callback);

    void get_app_version_details_async(const std::string& package_name, 
                                       const std::string& version_string, 
                                       const std::string& lang, 
                                       std::function<void(std::optional<AppVersionDetails>)> callback);

    void get_app_reviews_async(const std::string& package_name, 
                               const std::string& version_string, 
                               std::function<void(std::optional<std::vector<Review>>)> callback);

    void post_app_review_async(const std::string& package_name, 
                               const std::string& version_string, 
                               int rating, 
                               const std::string& comment, 
                               std::function<void(bool)> callback);

    void get_categories_async(std::function<void(std::optional<std::vector<Category>>)> callback);

    void download_image_async(const std::string& image_path, std::function<void(std::optional<std::string>)> callback);

    void clear_cache();

private:
    struct Private;
    Private* d;
};

} // namespace horizon::apt
