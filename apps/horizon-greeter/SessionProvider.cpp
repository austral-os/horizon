#include "SessionProvider.hpp"
#include <filesystem>
#include <fstream>
#include <horizon/Logger.hpp>

namespace horizon::greeter
{
    std::vector<SessionInfo> SessionProvider::get_sessions()
    {
        std::vector<SessionInfo> sessions;
        std::string sessions_dir = "/usr/share/wayland-sessions";

        if (!std::filesystem::exists(sessions_dir))
        {
            LOG_ERROR << "SessionProvider: /usr/share/wayland-sessions does not exist.";
            return sessions;
        }

        for (const auto &entry : std::filesystem::directory_iterator(sessions_dir))
        {
            if (entry.path().extension() == ".desktop")
            {
                std::ifstream f(entry.path());
                if (!f.is_open())
                    continue;

                SessionInfo session;
                std::string line;
                bool desktop_entry_section = false;

                while (std::getline(f, line))
                {
                    if (line == "[Desktop Entry]")
                    {
                        desktop_entry_section = true;
                        continue;
                    }
                    if (line.empty() || line[0] == '#')
                        continue;
                    if (line[0] == '[' && line != "[Desktop Entry]")
                    {
                        desktop_entry_section = false;
                        continue;
                    }

                    if (desktop_entry_section)
                    {
                        if (line.substr(0, 5) == "Name=")
                        {
                            session.name = line.substr(5);
                        }
                        else if (line.substr(0, 5) == "Exec=")
                        {
                            std::string exec = line.substr(5);
                            // Remove common desktop macros like %u, %f, %F, etc.
                            size_t percent = exec.find('%');
                            if (percent != std::string::npos)
                            {
                                exec = exec.substr(0, percent);
                            }
                            // Trim trailing whitespace
                            exec.erase(exec.find_last_not_of(" \t\n\r\f\v") + 1);
                            session.exec = exec;
                        }
                        else if (line.substr(0, 8) == "Comment=")
                        {
                            session.comment = line.substr(8);
                        }
                    }
                }

                if (!session.name.empty() && !session.exec.empty())
                {
                    sessions.push_back(session);
                }
            }
        }

        LOG_INFO << "SessionProvider: Found " << sessions.size() << " sessions.";
        for (const auto &s : sessions)
        {
            LOG_INFO << "  - " << s.name << " (" << s.exec << ")";
        }

        return sessions;
    }
} // namespace horizon::greeter
