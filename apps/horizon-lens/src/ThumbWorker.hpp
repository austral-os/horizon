#pragma once

#include <horizon/lens/ThumbnailCache.hpp>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

namespace horizon::lens
{
    /**
     * @struct ThumbRequest
     * @brief A single thumbnail generation request in the work queue.
     */
    struct ThumbRequest
    {
        std::string    path;
        ThumbnailSize  size{ThumbnailSize::Large};
        bool           high_priority{false};
    };

    /**
     * @class ThumbWorker
     * @brief Consumes ThumbRequests from a queue and generates thumbnails at low priority.
     *
     * Features:
     * - Runs in a dedicated thread with SCHED_IDLE / nice(19) scheduling.
     * - Reads high-priority requests from the inotify-watched queue directory.
     * - Skips already-valid thumbnails (verified via mtime check).
     * - Throttles itself by reading /proc/loadavg every 10 seconds.
     * - Generates thumbnails for all three standard sizes when a file is enqueued.
     */
    class ThumbWorker
    {
    public:
        ThumbWorker();
        ~ThumbWorker();

        /**
         * @brief Enqueues a file path for thumbnail generation (from scanner thread).
         * Thread-safe.
         */
        void enqueue(const std::string& path, ThumbnailSize size = ThumbnailSize::Large,
                     bool high_priority = false);

        /**
         * @brief Returns the current queue size.
         */
        size_t queue_size() const;

        /**
         * @brief Starts the worker thread.
         */
        void start();

        /**
         * @brief Signals the worker to drain the queue and stop.
         */
        void stop();

        /**
         * @brief Base pause between thumbnail generations in milliseconds. Default: 50ms.
         * Automatically increases under system load.
         */
        void set_base_pause_ms(int ms) { m_base_pause_ms = ms; }

    private:
        void worker_thread_fn();
        void apply_low_priority();
        void process_queue_dir();
        int  compute_adaptive_pause_ms();
        float read_system_load();

        std::deque<ThumbRequest>      m_queue;
        mutable std::mutex            m_queue_mutex;
        std::condition_variable       m_cv;
        std::thread                   m_thread;
        std::atomic<bool>             m_stop{false};
        std::unordered_set<std::string> m_in_flight; ///< Paths currently being processed

        int   m_base_pause_ms{50};
        float m_load_threshold{2.0f};

        std::string m_queue_dir; ///< Path to ~/.cache/horizon/thumbnails/queue/
    };

} // namespace horizon::lens
