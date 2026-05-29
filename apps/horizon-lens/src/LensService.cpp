#include "LensService.hpp"
#include "LensDbusService.hpp"
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

    // Initialize D-Bus
    try {
        m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SESSION);
        if (m_dbus->request_name("org.horizon.Lens")) {
            m_dbus_service = std::make_unique<LensDbusService>(*m_dbus, *m_worker);
            m_dbus->register_fallback("/org/horizon/Lens", m_dbus_service.get());
            LOG_INFO << "horizon-lens: D-Bus service registered at org.horizon.Lens";
        } else {
            LOG_ERROR << "horizon-lens: Failed to acquire D-Bus name org.horizon.Lens";
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "horizon-lens: Failed to initialize D-Bus: " << e.what();
    }
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

    // Main thread event loop
    while (m_running) {
        if (m_dbus) {
            m_dbus->process_events(100); // 100ms timeout
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
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
