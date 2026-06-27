#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace horizon::lens
{
    /**
     * @class DirectoryScanner
     * @brief Recursively scans the user's home directory in a low-priority background thread.
     *
     * Produces a stream of file paths (images and PDFs) that are fed into the ThumbWorker.
     * Uses breadth-first traversal with configurable inter-directory pauses to keep CPU
     * usage minimal. The scan repeats every RESCAN_INTERVAL_MINUTES after completion.
     *
     * Excluded directories (hardcoded defaults):
     *   .git, node_modules, .cache, .local/share/Trash, proc, sys, dev
     */
    class DirectoryScanner
    {
    public:
        /**
         * @brief Callback invoked for each discovered file candidate.
         * Called from the scanner thread — the callback must be thread-safe.
         */
        using FileFoundCallback = std::function<void(const std::string& path)>;

        explicit DirectoryScanner(const std::string& root_path);
        ~DirectoryScanner();

        /**
         * @brief Sets the callback to receive discovered file paths.
         */
        void set_on_file_found(FileFoundCallback cb) { m_on_file_found = cb; }

        /**
         * @brief Adds an additional directory path to be scanned in the background.
         */
        void add_scan_path(const std::string& path);


        /**
         * @brief Starts the background scanning thread.
         */
        void start();

        /**
         * @brief Signals the scanner to stop and waits for the thread to finish.
         */
        void stop();

        /**
         * @brief Milliseconds to sleep between processing each directory entry.
         * Higher values = less CPU impact. Default: 5ms.
         */
        void set_inter_entry_pause_ms(int ms) { m_inter_entry_pause_ms = ms; }

        /**
         * @brief Minutes to wait before re-scanning after a full pass. Default: 30.
         */
        void set_rescan_interval_minutes(int minutes) { m_rescan_interval_minutes = minutes; }

    private:
        void scanner_thread_fn();
        void scan_directory(const std::string& path);
        bool is_excluded(const std::string& path) const;
        bool is_candidate(const std::string& path) const;

        std::vector<std::string> m_scan_paths;

        FileFoundCallback m_on_file_found;
        std::thread       m_thread;
        std::atomic<bool> m_stop{false};
        std::condition_variable m_cv;
        std::mutex        m_cv_mutex;
        int               m_inter_entry_pause_ms{5};
        int               m_rescan_interval_minutes{30};

        static const std::vector<std::string> EXCLUDED_DIRS;
        static const std::vector<std::string> SUPPORTED_EXTENSIONS;
    };

} // namespace horizon::lens
