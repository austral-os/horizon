#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace horizon::arkutils
{
    /**
     * @enum WatchEvent
     * @brief Types of events monitored by DirectoryWatcher.
     */
    enum class WatchEvent
    {
        Created,
        Deleted,
        Modified,
        MovedFrom,
        MovedTo,
        AttributeChanged
    };

    /**
     * @class DirectoryWatcher
     * @brief Monitors directory changes using Linux inotify.
     */
    class DirectoryWatcher
    {
    public:
        using Callback = std::function<void(const std::string &path, const std::string &filename,
                                            WatchEvent event)>;

        DirectoryWatcher();
        ~DirectoryWatcher();

        /**
         * @brief Starts watching a directory.
         * @param path The directory path to watch.
         * @return True if watch was successfully added.
         */
        bool watch(const std::string &path);

        /**
         * @brief Stops watching a directory.
         * @param path The directory path to stop watching.
         */
        void unwatch(const std::string &path);

        /**
         * @brief Stops watching all directories.
         */
        void unwatch_all();

        /**
         * @brief Sets the callback for filesystem events.
         */
        void set_callback(Callback callback);

        /**
         * @brief Starts the background monitoring thread.
         */
        void start();

        /**
         * @brief Stops the background monitoring thread.
         */
        void stop();

    private:
        void run();

        int m_fd = -1;
        std::map<int, std::string> m_watch_descriptors;
        std::map<std::string, int> m_path_to_wd;
        std::mutex m_mutex;
        Callback m_callback;
        std::thread m_thread;
        std::atomic<bool> m_running{false};
    };
} // namespace horizon::arkutils
