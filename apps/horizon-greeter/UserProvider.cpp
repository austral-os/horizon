#include "UserProvider.hpp"
#include <filesystem>
#include <fstream>
#include <horizon/Logger.hpp>
#include <nlohmann/json.hpp>
#include <pwd.h>
#include <unistd.h>

namespace horizon::greeter
{
    std::vector<UserInfo> UserProvider::get_users()
    {
        std::vector<UserInfo> users;
        struct passwd *pw;

        setpwent();
        while ((pw = getpwent()) != nullptr)
        {
            // Filter real users
            if (pw->pw_uid < 1000 || pw->pw_uid > 60000)
                continue;

            // Filter system users or nologin
            std::string shell = pw->pw_shell;
            if (shell.find("nologin") != std::string::npos || shell.find("false") != std::string::npos)
                continue;

            UserInfo user;
            user.username = pw->pw_name;
            user.real_name = pw->pw_gecos;
            // Remove everything after the first comma in GECOS (common in /etc/passwd)
            size_t comma = user.real_name.find(',');
            if (comma != std::string::npos)
            {
                user.real_name = user.real_name.substr(0, comma);
            }
            if (user.real_name.empty())
            {
                user.real_name = user.username;
            }

            user.avatar_path = get_user_avatar(user.username);
            user.wallpaper_path = get_user_wallpaper(user.username);

            users.push_back(user);
        }
        endpwent();

        LOG_INFO << "UserProvider: Found " << users.size() << " users.";
        for (const auto &u : users)
        {
            LOG_INFO << "  - " << u.username << " (" << u.real_name << "), wallpaper: " << u.wallpaper_path;
        }

        return users;
    }

    std::string UserProvider::get_user_wallpaper(const std::string &username)
    {
        std::error_code ec;
        std::string config_path = "/home/" + username + "/.config/horizon/desktop.json";
        if (std::filesystem::exists(config_path, ec) && !ec)
        {
            try
            {
                std::ifstream f(config_path);
                nlohmann::json j;
                f >> j;

                if (j.contains("desktop") && j["desktop"].contains("backgrounds") &&
                    j["desktop"]["backgrounds"].contains("current") &&
                    j["desktop"]["backgrounds"]["current"].contains("path"))
                {
                    std::string path = j["desktop"]["backgrounds"]["current"]["path"];
                    if (std::filesystem::exists(path, ec) && !ec)
                    {
                        return path;
                    }
                }
            }
            catch (...)
            {
                // Ignore parsing errors
            }
        }

        // Default backgrounds
        std::string default_dir = "/usr/share/horizon/backgrounds";
        if (std::filesystem::exists(default_dir, ec) && !ec && std::filesystem::is_directory(default_dir, ec) && !ec)
        {
            for (const auto &entry : std::filesystem::directory_iterator(default_dir, ec))
            {
                if (ec) break;
                if (entry.is_regular_file())
                {
                    return entry.path().string();
                }
            }
        }

        return "";
    }

    std::string UserProvider::get_user_avatar(const std::string &username)
    {
        std::error_code ec;
        // 1. Check ~/.face
        std::string dot_face = "/home/" + username + "/.face";
        if (std::filesystem::exists(dot_face, ec) && !ec)
            return dot_face;

        // 2. Check AccountsService
        std::string acc_service = "/var/lib/AccountsService/users/" + username;
        if (std::filesystem::exists(acc_service, ec) && !ec)
        {
            std::ifstream f(acc_service);
            std::string line;
            while (std::getline(f, line))
            {
                if (line.find("Icon=") == 0)
                {
                    return line.substr(5);
                }
            }
        }

        // Return empty so UI can use a default icon
        return "";
    }
} // namespace horizon::greeter
