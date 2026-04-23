#pragma once

#include <string>
#include <vector>

namespace horizon::greeter
{
    /**
     * @struct UserInfo
     * @brief Data about a system user.
     */
    struct UserInfo
    {
        std::string username;
        std::string real_name;
        std::string avatar_path;
        std::string wallpaper_path;
    };

    /**
     * @class UserProvider
     * @brief Discovers system users and their personal configurations.
     */
    class UserProvider
    {
    public:
        static std::vector<UserInfo> get_users();

    private:
        static std::string get_user_wallpaper(const std::string &username);
        static std::string get_user_avatar(const std::string &username);
    };
} // namespace horizon::greeter
