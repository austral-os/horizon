#include "horizon/ApplicationLauncher.hpp"
#include "horizon/DesktopEntry.hpp"
#include "horizon/Logger.hpp"
#include <fcntl.h>
#include <filesystem>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace horizon
{
    static bool is_binary_in_path(const std::string &name)
    {
        const char *path_env = std::getenv("PATH");
        if (!path_env)
            return false;

        std::string path_str(path_env);
        std::istringstream ss(path_str);
        std::string dir;

        while (std::getline(ss, dir, ':'))
        {
            if (dir.empty())
                continue;
            fs::path p = fs::path(dir) / name;
            if (fs::exists(p) &&
                (fs::status(p).permissions() & (fs::perms::owner_exec | fs::perms::group_exec |
                                                fs::perms::others_exec)) != fs::perms::none)
            {
                return true;
            }
        }
        return false;
    }

    bool ApplicationLauncher::launch(const std::string &name)
    {
        LOG_INFO << "[ApplicationLauncher] Attempting to launch by name: " << name;

        if (is_binary_in_path(name))
        {
            LOG_INFO << "[ApplicationLauncher] Found '" << name
                     << "' in PATH, launching as binary.";
            return launch_binary(name);
        }

        LOG_INFO << "[ApplicationLauncher] '" << name
                 << "' not in PATH, falling back to desktop file.";
        return launch_from_desktop_file(name);
    }

    bool ApplicationLauncher::launch_from_desktop_file(const std::string &path_or_id)
    {
        std::string path = path_or_id;
        if (!fs::exists(path) || !fs::is_regular_file(path))
        {
            path = DesktopEntry::find_desktop_file(path_or_id);
        }

        if (path.empty())
        {
            LOG_ERROR << "[ApplicationLauncher] Could not find desktop file for: " << path_or_id;
            return false;
        }

        std::string command = DesktopEntry::get_exec_command_from_path(path);
        if (command.empty())
        {
            LOG_ERROR << "[ApplicationLauncher] No Exec command found in: " << path;
            return false;
        }

        // Remove field codes like %u, %F, etc.
        std::string cleaned_command;
        std::istringstream iss(command);
        std::string token;
        while (iss >> token)
        {
            if (token[0] == '%')
                continue;
            if (!cleaned_command.empty())
                cleaned_command += " ";
            cleaned_command += token;
        }

        auto parts = split_command(cleaned_command);
        if (parts.empty())
            return false;

        std::string binary = parts[0];
        parts.erase(parts.begin());

        return launch_binary(binary, parts);
    }

    bool ApplicationLauncher::launch_binary(const std::string &path,
                                            const std::vector<std::string> &args)
    {
        LOG_INFO << "[ApplicationLauncher] Launching: " << path;

        pid_t pid = fork();
        if (pid < 0)
        {
            LOG_ERROR << "[ApplicationLauncher] First fork failed";
            return false;
        }

        if (pid == 0)
        {
            // First child
            setsid(); // Start a new session

            pid_t pid2 = fork();
            if (pid2 < 0)
            {
                _exit(1);
            }

            if (pid2 == 0)
            {
                // Grandchild (the actual application)
                // Redirect I/O to /dev/null to fully detach
                int dev_null = open("/dev/null", O_RDWR);
                if (dev_null != -1)
                {
                    dup2(dev_null, STDIN_FILENO);
                    dup2(dev_null, STDOUT_FILENO);
                    dup2(dev_null, STDERR_FILENO);
                    close(dev_null);
                }

                // Prepare arguments
                std::vector<char *> argv;
                argv.push_back(const_cast<char *>(path.c_str()));
                for (const auto &arg : args)
                {
                    argv.push_back(const_cast<char *>(arg.c_str()));
                }
                argv.push_back(nullptr);

                execvp(path.c_str(), argv.data());

                // If execvp returns, it failed
                LOG_ERROR << "[ApplicationLauncher] execvp failed for: " << path;
                _exit(1);
            }
            else
            {
                // First child exits immediately, grandchild is adopted by init
                _exit(0);
            }
        }

        // Parent waits for the first child to exit
        int status;
        waitpid(pid, &status, 0);

        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    std::vector<std::string> ApplicationLauncher::split_command(const std::string &command)
    {
        std::vector<std::string> tokens;
        std::string token;
        bool in_quotes = false;
        char quote_char = 0;

        for (size_t i = 0; i < command.length(); ++i)
        {
            char c = command[i];
            if (in_quotes)
            {
                if (c == quote_char)
                {
                    in_quotes = false;
                    tokens.push_back(token);
                    token.clear();
                }
                else
                {
                    token += c;
                }
            }
            else
            {
                if (c == '"' || c == '\'')
                {
                    in_quotes = true;
                    quote_char = c;
                }
                else if (std::isspace(c))
                {
                    if (!token.empty())
                    {
                        tokens.push_back(token);
                        token.clear();
                    }
                }
                else
                {
                    token += c;
                }
            }
        }

        if (!token.empty())
        {
            tokens.push_back(token);
        }

        return tokens;
    }
} // namespace horizon
