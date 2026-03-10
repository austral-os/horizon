#pragma once

#include <string>
#include <vector>

namespace horizon
{
    /**
     * @class ApplicationLauncher
     * @brief Utility class for launching applications independently of the parent process.
     */
    class ApplicationLauncher
    {
    public:
        /**
         * @brief Launches an application by name. It first tries to find a binary in PATH,
         * then falls back to searching for a .desktop file.
         * @param name The application name or ID.
         * @return True if the launch was successful.
         */
        static bool launch(const std::string &name);

        /**
         * @brief Launches an application using its .desktop file.
         * @param path_or_id Path to a .desktop file or an application ID.
         * @return True if the launch was successful.
         */
        static bool launch_from_desktop_file(const std::string &path_or_id);

        /**
         * @brief Launches an application from a binary path and arguments.
         * @param path Path to the executable.
         * @param args List of arguments for the application.
         * @return True if the launch was successful.
         */
        static bool launch_binary(const std::string &path,
                                  const std::vector<std::string> &args = {});

    private:
        /**
         * @brief Splits a command string into binary and arguments, handling quotes.
         * @param command The full command string.
         * @return A vector where the first element is the binary and others are args.
         */
        static std::vector<std::string> split_command(const std::string &command);
    };
} // namespace horizon
