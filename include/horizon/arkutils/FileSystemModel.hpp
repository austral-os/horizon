#pragma once

#include <horizon/SignalManager.hpp>
#include <horizon/arkutils/DirectoryWatcher.hpp>
#include <horizon/arkutils/FileInfo.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace horizon::arkutils
{
    /**
     * @class FileSystemModel
     * @brief High-level component that manages file listing, metadata caching, and change
     * notifications.
     */
    class FileSystemModel
    {
    public:
        FileSystemModel();
        ~FileSystemModel();

        /**
         * @brief Lists files in a directory. Uses cache if available.
         * @param path Directory path.
         * @param force_refresh If true, bypasses the cache.
         * @return Vector of FileInfo.
         */
        std::vector<FileInfo> list_directory(const std::string &path, bool force_refresh = false);

        /**
         * @brief Returns metadata for a single item.
         */
        FileInfo get_info(const std::string &path);

        /**
         * @brief Signal identifier for when a directory changes.
         * "fs_directory_changed" -> SignalContext.data will point to the path (std::string*).
         */
        static constexpr const char *SIGNAL_DIRECTORY_CHANGED = "fs_directory_changed";

        /**
         * @brief Access to the signal manager for notifications.
         */
        SignalManager &signal_manager()
        {
            return m_signals;
        }

        /**
         * @brief Stops watching all currently watched directories.
         */
        void unwatch_all();

    private:
        void on_watcher_event(const std::string &path, const std::string &filename,
                              WatchEvent event);
        void refresh_cache(const std::string &path);

        std::map<std::string, std::vector<FileInfo>> m_cache;
        std::mutex m_cache_mutex;

        std::unique_ptr<DirectoryWatcher> m_watcher;
        SignalManager m_signals;
    };
} // namespace horizon::arkutils
