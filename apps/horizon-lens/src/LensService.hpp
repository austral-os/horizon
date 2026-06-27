#pragma once

#include "DirectoryScanner.hpp"
#include "ThumbWorker.hpp"
#include <horizon/dbusutils/DbusHelper.hpp>
#include <atomic>
#include <memory>
#include <string>

namespace horizon::lens
{
    class LensDbusService;

    /**
     * @class LensService
     * @brief Orchestrates the background thumbnail generation service.
     *
     * Owns the DirectoryScanner and ThumbWorker. Handles graceful shutdown
     * via SIGTERM/SIGINT. All heavy work happens in worker threads; the main
     * thread just waits for a stop signal.
     */
    class LensService
    {
    public:
        LensService();
        ~LensService();

        /**
         * @brief Starts the service and blocks until stop() is called or a signal arrives.
         */
        void run();

        /**
         * @brief Requests a graceful shutdown of all worker threads.
         */
        void stop();

        static LensService* instance() { return s_instance; }
        DirectoryScanner* get_scanner() const { return m_scanner.get(); }

    private:
        std::unique_ptr<DirectoryScanner> m_scanner;
        std::unique_ptr<ThumbWorker>      m_worker;
        
        std::unique_ptr<dbusutils::DbusHelper> m_dbus;
        std::unique_ptr<LensDbusService>       m_dbus_service;

        std::atomic<bool>                 m_running{false};

        void setup_signal_handlers();
        static void signal_handler(int sig);
        static LensService* s_instance;
    };

} // namespace horizon::lens
