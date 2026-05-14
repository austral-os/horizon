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
    static std::string run_command_capture_output(const std::string &cmd)
    {
        char buffer[128];
        std::string result = "";
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            return "";
        }
        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
        {
            result += buffer;
        }
        int exit_code = pclose(pipe);
        if (exit_code != 0)
            return "";

        // Trim newline and carriage return
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        {
            result.pop_back();
        }
        return result;
    }

    static bool is_binary_in_path(const std::string &name)
    {
        if (name.empty())
            return false;

        // If it's an absolute or relative path, check it directly
        if (name.find('/') != std::string::npos)
        {
            fs::path p(name);
            return fs::exists(p) &&
                   (fs::status(p).permissions() & (fs::perms::owner_exec | fs::perms::group_exec |
                                                   fs::perms::others_exec)) != fs::perms::none;
        }

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
        LOG_INFO << "[ApplicationLauncher] Attempting to launch: " << name;

        if (name.size() > 8 && name.substr(name.size() - 8) == ".desktop")
        {
            LOG_INFO << "[ApplicationLauncher] '" << name
                     << "' is a desktop file, launching via desktop entry logic.";
            return launch_from_desktop_file(name);
        }

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

    bool ApplicationLauncher::launch_from_desktop_file(const std::string &path_or_id,
                                                       const std::vector<std::string> &args)
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

        std::string working_dir = DesktopEntry::get_value_from_desktop_file(path, "Path");

        // Handle field codes like %u, %F, etc.
        // %f: single file name
        // %F: multiple file names
        // %u: single URL
        // %U: multiple URLs

        std::string arg_concat = "";
        for (const auto &arg : args)
        {
            if (!arg_concat.empty())
                arg_concat += " ";
            arg_concat += "\"" + arg + "\"";
        }

        bool field_code_replaced = false;
        std::string working_command = command;
        size_t pos = 0;

        while ((pos = working_command.find('%', pos)) != std::string::npos &&
               pos + 1 < working_command.length())
        {
            char code = working_command[pos + 1];
            std::string replacement = "";
            bool found_code = true;

            if (code == 'f' || code == 'u')
            {
                replacement = args.empty() ? "" : "\"" + args[0] + "\"";
                field_code_replaced = true;
            }
            else if (code == 'F' || code == 'U')
            {
                replacement = arg_concat;
                field_code_replaced = true;
            }
            else if (code == 'i' || code == 'c')
            {
                // Icon or Name, ignored for now
                replacement = "";
            }
            else if (code == 'k')
            {
                // Desktop file path
                replacement = "\"" + path + "\"";
            }
            else if (code == '%')
            {
                // Literal %
                replacement = "%";
            }
            else
            {
                found_code = false;
            }

            if (found_code)
            {
                working_command.replace(pos, 2, replacement);
                pos += replacement.length();
            }
            else
            {
                pos++;
            }
        }

        std::string final_command = working_command;

        // If NO field codes were found but we have arguments, append them at the end.
        if (!field_code_replaced && !args.empty())
        {
            if (!final_command.empty() && final_command.back() != ' ')
                final_command += " ";
            final_command += arg_concat;
        }

        auto parts = split_command(final_command);
        if (parts.empty())
            return false;

        std::string binary = parts[0];
        parts.erase(parts.begin());

        if (binary.empty())
        {
            LOG_ERROR << "[ApplicationLauncher] Extracted binary name is empty from command: " << final_command;
            return false;
        }

        return launch_binary(binary, parts, working_dir);
    }

    bool ApplicationLauncher::launch_binary(const std::string &path,
                                            const std::vector<std::string> &args,
                                            const std::string &working_dir)
    {
        LOG_INFO << "[ApplicationLauncher] Launching: " << path;
        if (!working_dir.empty())
        {
            LOG_INFO << "[ApplicationLauncher] Working directory: " << working_dir;
        }

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

                if (!working_dir.empty())
                {
                    if (chdir(working_dir.c_str()) != 0)
                    {
                        LOG_ERROR << "[ApplicationLauncher] chdir failed for: " << working_dir;
                    }
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

                // If execvp returns, it failed. 
                // Note: we can't use LOG_ERROR here effectively because stderr is /dev/null, 
                // but the system error might be useful if we ever change that.
                // However, we can write to a specific emergency log.
                int err = errno;
                FILE* f = fopen("/tmp/horizon_launch_error.log", "a");
                if (f) {
                    fprintf(f, "[ApplicationLauncher] execvp failed for: %s (errno: %d)\n", path.c_str(), err);
                    fclose(f);
                }
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
    bool ApplicationLauncher::open_file(const std::string &path)
    {
        LOG_INFO << "[ApplicationLauncher] Opening file: " << path;

        // 1. Get MIME type
        std::string mime_type = run_command_capture_output("xdg-mime query filetype \"" + path + "\"");
        if (mime_type.empty())
        {
            LOG_ERROR << "[ApplicationLauncher] Could not determine MIME type for: " << path;
            return false;
        }
        LOG_INFO << "[ApplicationLauncher] MIME type: " << mime_type;

        // 2. Get default application
        std::string desktop_id =
            run_command_capture_output("xdg-mime query default \"" + mime_type + "\"");
        if (desktop_id.empty())
        {
            LOG_ERROR << "[ApplicationLauncher] No default application for MIME type: " << mime_type;
            return false;
        }
        LOG_INFO << "[ApplicationLauncher] Default application ID: " << desktop_id;

        // 3. Launch from desktop file
        return launch_from_desktop_file(desktop_id, {path});
    }
} // namespace horizon
