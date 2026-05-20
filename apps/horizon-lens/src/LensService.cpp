#include "LensService.hpp"
#include <horizon/Logger.hpp>

#include <csignal>
#include <thread>
#include <chrono>

namespace horizon::lens
{

LensService* LensService::s_instance = nullptr;

// ---------------------------------------------------------------------------
// Signal handler
// ---------------------------------------------------------------------------
void LensService::signal_handler(int sig)
{
    LOG_INFO << "horizon-lens: Received signal " << sig << ", shutting down...";
    if (s_instance) s_instance->stop();
}

void LensService::setup_signal_handlers()
{
    s_instance = this;
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGHUP,  signal_handler);
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
LensService::LensService()
{
    const char* home = std::getenv("HOME");
    std::string root = home ? home : "/";

    m_scanner = std::make_unique<DirectoryScanner>(root);
    m_worker  = std::make_unique<ThumbWorker>();

    // Wire scanner output → worker input
    m_scanner->set_on_file_found([this](const std::string& path) {
        // Enqueue for all sizes (worker will skip already-cached ones)
        m_worker->enqueue(path, ThumbnailSize::Large, false);
    });
}

LensService::~LensService()
{
    stop();
}

// ---------------------------------------------------------------------------
// Run / Stop
// ---------------------------------------------------------------------------
void LensService::run()
{
    setup_signal_handlers();

    LOG_INFO << "horizon-lens: Starting service...";
    m_running = true;

    m_worker->start();
    m_scanner->start();

    LOG_INFO << "horizon-lens: Service running. Queue is processed in background.";

    // Main thread just waits; all work happens in scanner/worker threads
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOG_INFO << "horizon-lens: Stopping service...";
    m_scanner->stop();
    m_worker->stop();
    LOG_INFO << "horizon-lens: Service stopped cleanly.";
}

void LensService::stop()
{
    m_running = false;
}

} // namespace horizon::lens
