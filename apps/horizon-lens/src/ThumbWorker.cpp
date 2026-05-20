#include "ThumbWorker.hpp"
#include "ThumbGenerator.hpp"
#include <horizon/Logger.hpp>
#include <horizon/lens/ThumbnailCache.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <malloc.h>   // malloc_trim
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>

// nlohmann/json for queue file parsing
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Maximum number of pending paths in the work queue.
// When this limit is reached, enqueue() becomes a no-op until the worker drains entries.
// This bounds RAM usage: 300 × ~64 bytes per string ≈ ~20 KB max for the queue itself.
static constexpr size_t MAX_QUEUE_SIZE = 300;

namespace horizon::lens
{

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
ThumbWorker::ThumbWorker()
{
    const char* home = std::getenv("HOME");
    if (home) {
        m_queue_dir = std::string(home) + "/.cache/horizon/thumbnails/queue";
        std::error_code ec;
        fs::create_directories(m_queue_dir, ec);
    }
}

ThumbWorker::~ThumbWorker()
{
    stop();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void ThumbWorker::start()
{
    m_stop = false;
    m_thread = std::thread(&ThumbWorker::worker_thread_fn, this);
}

void ThumbWorker::stop()
{
    m_stop = true;
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

void ThumbWorker::enqueue(const std::string& path, ThumbnailSize size, bool high_priority)
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);

    // Avoid duplicates already in queue or being processed
    if (m_in_flight.count(path)) return;

    // --- RAM cap ---
    // Background (low-priority) items are dropped when the queue is full.
    // High-priority items (explicit app requests) always go through.
    if (!high_priority && m_queue.size() >= MAX_QUEUE_SIZE)
        return;

    ThumbRequest req{path, size, high_priority};
    if (high_priority) {
        m_queue.push_front(req);
    } else {
        m_queue.push_back(req);
    }
    m_cv.notify_one();
}

size_t ThumbWorker::queue_size() const
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    return m_queue.size();
}

// ---------------------------------------------------------------------------
// Thread priority
// ---------------------------------------------------------------------------
void ThumbWorker::apply_low_priority()
{
    // Set SCHED_IDLE for the worker thread — absolute lowest priority
    struct sched_param param{};
    param.sched_priority = 0;
    pthread_setschedparam(pthread_self(), SCHED_IDLE, &param);

    // Also apply nice(19) as fallback
    setpriority(PRIO_PROCESS, 0, 19);
}

// ---------------------------------------------------------------------------
// Load-adaptive throttling
// ---------------------------------------------------------------------------
float ThumbWorker::read_system_load()
{
    std::ifstream f("/proc/loadavg");
    float load1 = 0.0f;
    if (f) f >> load1;
    return load1;
}

int ThumbWorker::compute_adaptive_pause_ms()
{
    float load = read_system_load();
    if (load >= m_load_threshold * 2.0f)  return m_base_pause_ms * 10; // Very high load
    if (load >= m_load_threshold)          return m_base_pause_ms * 4;  // High load
    if (load >= m_load_threshold * 0.5f)   return m_base_pause_ms * 2;  // Moderate
    return m_base_pause_ms;                                              // Normal
}

// ---------------------------------------------------------------------------
// Queue directory (high-priority requests from apps)
// ---------------------------------------------------------------------------
void ThumbWorker::process_queue_dir()
{
    if (m_queue_dir.empty()) return;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(m_queue_dir, ec)) {
        if (m_stop) return;
        if (entry.path().extension() != ".req") continue;

        std::string req_path = entry.path().string();
        std::ifstream f(req_path);
        if (!f) continue;

        try {
            json j = json::parse(f);
            f.close();

            std::string path     = j.value("path", "");
            std::string size_str = j.value("size", "large");

            ThumbnailSize size = ThumbnailSize::Large;
            if (size_str == "normal")  size = ThumbnailSize::Normal;
            if (size_str == "x-large") size = ThumbnailSize::XLarge;

            if (!path.empty())
                enqueue(path, size, /*high_priority=*/true);

        } catch (...) {}

        // Remove the request file regardless of parse success
        fs::remove(req_path, ec);
    }
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------
void ThumbWorker::worker_thread_fn()
{
    apply_low_priority();
    LOG_INFO << "horizon-lens: ThumbWorker started (nice 19 / SCHED_IDLE)";

    int load_check_counter = 0;
    int pause_ms = m_base_pause_ms;

    while (!m_stop) {
        // Check queue dir for high-priority requests every 5 items
        if (++load_check_counter >= 5) {
            load_check_counter = 0;
            process_queue_dir();
            pause_ms = compute_adaptive_pause_ms();
        }

        ThumbRequest req;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_cv.wait_for(lock, std::chrono::milliseconds(200),
                          [this] { return !m_queue.empty() || m_stop; });

            if (m_stop) break;
            if (m_queue.empty()) {
                // Nothing in queue — check the queue dir
                process_queue_dir();
                continue;
            }

            req = m_queue.front();
            m_queue.pop_front();
            m_in_flight.insert(req.path);
        }

        // --- Size selection strategy ---
        // Background scanner items only generate the Large (256×256) size.
        // This avoids loading the full image three times for Normal + Large + XLarge.
        // Normal and XLarge are generated only for explicit high-priority requests
        // (i.e., when an app calls ThumbnailCache::request_thumbnail()).
        std::vector<ThumbnailSize> sizes;
        if (req.high_priority) {
            // App explicitly requested: generate requested size only
            sizes.push_back(req.size);
        } else {
            // Background scan: only Large — cheapest useful size for arkfm grid
            sizes.push_back(ThumbnailSize::Large);
        }

        for (auto size : sizes) {
            if (m_stop) break;

            // Skip if already cached and valid
            if (!ThumbnailCache::get_thumbnail(req.path, size).empty())
                continue;

            bool ok = ThumbGenerator::generate(req.path, size);
            if (ok) {
                LOG_INFO << "horizon-lens: Generated "
                         << (size == ThumbnailSize::Normal ? "normal" :
                             size == ThumbnailSize::Large  ? "large"  : "x-large")
                         << " thumbnail for " << req.path;
            }
        }

        // Return freed image/surface memory to the OS immediately.
        // Without this, glibc's heap keeps the memory mapped for future allocations,
        // causing RSS to spike permanently after the first large image.
        ::malloc_trim(0);

        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_in_flight.erase(req.path);
        }

        // Throttle between files
        if (!m_stop)
            std::this_thread::sleep_for(std::chrono::milliseconds(pause_ms));
    }

    LOG_INFO << "horizon-lens: ThumbWorker stopped.";
}

} // namespace horizon::lens
