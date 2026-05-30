#pragma once

#include "PrintService.h"
#include "PrinterService.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>

namespace horizon::print {

class AsyncPrintService {
private:
    PrintService& m_printService;
    PrinterService& m_printerService;
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<std::function<void()>> m_queue;
    std::thread m_worker;
    std::atomic<bool> m_stop{false};

    void workerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return m_stop || !m_queue.empty(); });
                
                if (m_stop && m_queue.empty()) {
                    return;
                }
                
                task = std::move(m_queue.front());
                m_queue.pop();
            }
            // Ejecutar fuera del lock
            if (task) {
                task();
            }
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(task));
        }
        m_cv.notify_one();
    }

public:
    AsyncPrintService(PrintService& printService, PrinterService& printerService)
        : m_printService(printService), m_printerService(printerService) {
        m_worker = std::thread(&AsyncPrintService::workerLoop, this);
    }

    ~AsyncPrintService() {
        m_stop = true;
        m_cv.notify_all();
        if (m_worker.joinable()) {
            m_worker.join();
        }
    }

    // --- Métodos asíncronos para PrintService ---

    void submitJobAsync(const PrinterId& printerId, const PrintDocument& document, const PrintConfig& config, std::function<void(JobId)> onDone = nullptr) {
        // Hacemos copias de los datos para el thread worker
        enqueue([this, printerId, document, config, onDone]() {
            JobId id = m_printService.submitJob(printerId, document, config);
            if (onDone) {
                onDone(id);
            }
        });
    }

    void cancelJobAsync(const JobId& jobId) {
        enqueue([this, jobId]() {
            m_printService.cancelJob(jobId);
        });
    }

    void pollJobStateAsync(const JobId& jobId) {
        enqueue([this, jobId]() {
            m_printService.pollJobState(jobId);
        });
    }

    // --- Métodos asíncronos para PrinterService ---

    void listPrintersAsync(std::function<void(std::vector<Printer>)> onResult) {
        enqueue([this, onResult]() {
            auto printers = m_printerService.listPrinters();
            if (onResult) {
                onResult(printers);
            }
        });
    }

    void addPrinterAsync(const std::string& name, const std::string& uri, const PrintConfig& config, std::function<void(PrinterId)> onDone = nullptr) {
        enqueue([this, name, uri, config, onDone]() {
            PrinterId id = m_printerService.addPrinter(name, uri, config);
            if (onDone) {
                onDone(id);
            }
        });
    }

    void removePrinterAsync(const PrinterId& id) {
        enqueue([this, id]() {
            m_printerService.removePrinter(id);
        });
    }
};

} // namespace horizon::print
