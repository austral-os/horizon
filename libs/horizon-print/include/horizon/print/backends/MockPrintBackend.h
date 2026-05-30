#pragma once

#include <horizon/print/IPrintBackend.h>
#include <map>
#include <stdexcept>
#include <mutex>
#include <algorithm>

namespace horizon::print::backends {

class MockPrintBackend : public IPrintBackend {
private:
    std::vector<Printer> m_printers;
    std::map<JobId, JobState> m_jobs;
    int m_jobCounter = 1;
    int m_printerCounter = 1;
    std::mutex m_mutex;

public:
    std::vector<Printer> listPrinters() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_printers;
    }

    PrinterId addPrinter(const std::string& name, const std::string& uri, const PrintConfig& config) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        PrinterId id = "mock_printer_" + std::to_string(m_printerCounter++);
        m_printers.push_back({id, name, uri, PrinterSource::Installed});
        return id;
    }

    void removePrinter(const PrinterId& id) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_printers.erase(
            std::remove_if(m_printers.begin(), m_printers.end(),
                [&](const Printer& p) { return p.id == id; }),
            m_printers.end());
    }

    JobId submitJob(const PrinterId& printerId, const PrintDocument& document, const PrintConfig& config) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Simular validación básica de backend
        if (!document.isValid()) {
            throw std::invalid_argument("Documento PDF inválido o corrupto.");
        }

        bool printerExists = false;
        for (const auto& p : m_printers) {
            if (p.id == printerId) {
                printerExists = true;
                break;
            }
        }
        if (!printerExists) {
            throw std::runtime_error("Impresora no encontrada: " + printerId);
        }

        JobId jobId = "mock_job_" + std::to_string(m_jobCounter++);
        m_jobs[jobId] = JobState::QUEUED;
        return jobId;
    }

    void cancelJob(const JobId& jobId) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_jobs.find(jobId) != m_jobs.end()) {
            m_jobs[jobId] = JobState::ERROR; // Cancelled is mapped to error/terminal
        }
    }

    JobState getJobState(const JobId& jobId) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_jobs.find(jobId);
        if (it != m_jobs.end()) {
            return it->second;
        }
        throw std::runtime_error("Job no encontrado: " + jobId);
    }

    // Funciones exclusivas del mock para avanzar estados en los tests
    void mock_setJobState(const JobId& jobId, JobState state) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_jobs.find(jobId) != m_jobs.end()) {
            m_jobs[jobId] = state;
        }
    }
};

} // namespace horizon::print::backends
