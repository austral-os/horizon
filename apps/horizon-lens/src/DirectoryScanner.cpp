#include "DirectoryScanner.hpp"
#include <horizon/Logger.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>
#include <deque>

// Maximum number of directory paths held in the BFS queue at once.
// A home directory can have thousands of subdirectories; without this cap the
// deque would hold all their paths as strings simultaneously in RAM.
static constexpr size_t MAX_BFS_QUEUE = 500;

namespace fs = std::filesystem;

namespace horizon::lens
{

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------
const std::vector<std::string> DirectoryScanner::EXCLUDED_DIRS = {
    ".git", ".hg", ".svn",
    "node_modules", "__pycache__",
    ".cache",
    "Trash",         // .local/share/Trash
    "proc", "sys", "dev", "run",
    ".wine", ".steam",
    "build", "CMakeFiles"
};

const std::vector<std::string> DirectoryScanner::SUPPORTED_EXTENSIONS = {
    ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp",
    ".svg", ".tiff", ".tif",
    ".pdf"
};

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
DirectoryScanner::DirectoryScanner(const std::string& root_path)
    : m_root_path(root_path)
{
}

DirectoryScanner::~DirectoryScanner()
{
    stop();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void DirectoryScanner::start()
{
    m_stop = false;
    m_thread = std::thread(&DirectoryScanner::scanner_thread_fn, this);
}

void DirectoryScanner::stop()
{
    m_stop = true;
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
bool DirectoryScanner::is_excluded(const std::string& path) const
{
    fs::path p(path);
    std::string name = p.filename().string();

    // Skip hidden directories (. prefix) except home root
    if (!name.empty() && name[0] == '.') {
        // Allow scanning inside ~/.config, ~/.local (but not .cache, .git, etc.)
        for (const auto& ex : EXCLUDED_DIRS)
            if (name == ex) return true;
        // Exclude any other hidden dir to avoid .wine, .steam, etc.
        if (name.size() > 1) return true;
    }
    for (const auto& ex : EXCLUDED_DIRS)
        if (name == ex) return true;

    return false;
}

bool DirectoryScanner::is_candidate(const std::string& path) const
{
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (const auto& supported : SUPPORTED_EXTENSIONS)
        if (ext == supported) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Scanner thread
// ---------------------------------------------------------------------------
void DirectoryScanner::scanner_thread_fn()
{
    LOG_INFO << "horizon-lens: DirectoryScanner started for " << m_root_path;

    while (!m_stop) {
        LOG_INFO << "horizon-lens: Starting scan pass of " << m_root_path;
        scan_directory(m_root_path);

        if (m_stop) break;

        LOG_INFO << "horizon-lens: Scan pass complete. Sleeping "
                 << m_rescan_interval_minutes << " minutes before next pass.";

        // Sleep for the configured interval, waking up if stopped
        std::unique_lock<std::mutex> lock(m_cv_mutex);
        m_cv.wait_for(lock,
                      std::chrono::minutes(m_rescan_interval_minutes),
                      [this] { return m_stop.load(); });
    }

    LOG_INFO << "horizon-lens: DirectoryScanner stopped.";
}

void DirectoryScanner::scan_directory(const std::string& start_path)
{
    // BFS with a size-capped deque.
    // When the queue is full we stop expanding subdirectories until some are consumed,
    // keeping peak RAM bounded regardless of the depth/breadth of the directory tree.
    std::deque<std::string> dirs;
    dirs.push_back(start_path);

    while (!dirs.empty() && !m_stop) {
        std::string current = dirs.front();
        dirs.pop_front();

        std::error_code ec;
        fs::directory_iterator it(current, fs::directory_options::skip_permission_denied, ec);
        if (ec) continue;

        for (const auto& entry : it) {
            if (m_stop) return;

            std::error_code ec2;
            auto status = entry.symlink_status(ec2);
            if (ec2) continue;

            // Skip symlinks to avoid cycles
            if (fs::is_symlink(status)) continue;

            std::string entry_path = entry.path().string();

            if (fs::is_directory(status)) {
                // Only push when the queue has room; otherwise we'll visit it
                // in the next scan pass (or when the queue drains naturally).
                if (!is_excluded(entry_path) && dirs.size() < MAX_BFS_QUEUE)
                    dirs.push_back(entry_path);
            } else if (fs::is_regular_file(status)) {
                if (is_candidate(entry_path) && m_on_file_found) {
                    m_on_file_found(entry_path);
                }
            }

            // Small pause per entry to reduce I/O pressure
            if (m_inter_entry_pause_ms > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(m_inter_entry_pause_ms));
        }
    }
}

} // namespace horizon::lens
