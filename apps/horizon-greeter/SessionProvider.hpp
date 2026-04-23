#pragma once

#include <string>
#include <vector>

namespace horizon::greeter
{
    /**
     * @struct SessionInfo
     * @brief Data about a Wayland session.
     */
    struct SessionInfo
    {
        std::string name;
        std::string exec;
        std::string comment;
    };

    /**
     * @class SessionProvider
     * @brief Discovers available Wayland sessions from /usr/share/wayland-sessions.
     */
    class SessionProvider
    {
    public:
        static std::vector<SessionInfo> get_sessions();
    };
} // namespace horizon::greeter
