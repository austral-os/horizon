#pragma once

#include "IPrintBackend.h"
#include <memory>
#include <functional>
#include <map>
#include <mutex>

namespace horizon::print {

class PrintService {
private:
    std::shared_ptr<IPrintBackend> m_backend;

public:
    // Callbacks planos para eventos
    std::function<void(const JobId&)> when_job_started;
    std::function<void(const JobId&)> when_job_finished;
    std::function<void(const JobId&, const std::string&)> when_job_error;

    explicit PrintService(std::shared_ptr<IPrintBackend> backend) : m_backend(std::move(backend)) {}

    JobId submitJob(const PrinterId& printerId, const PrintDocument& document, const PrintConfig& config) {
        if (!document.isValid()) {
            if (when_job_error) {
                when_job_error("", "Documento PDF inválido.");
            }
            return "";
        }

        try {
            JobId id = m_backend->submitJob(printerId, document, config);
            if (when_job_started) {
                when_job_started(id);
            }
            return id;
        } catch (const std::exception& e) {
            if (when_job_error) {
                when_job_error("", e.what());
            }
            return "";
        }
    }

    void cancelJob(const JobId& id) {
        m_backend->cancelJob(id);
    }

    JobState getJobState(const JobId& id) const {
        return m_backend->getJobState(id);
    }

    // Método utilitario para que los clientes o wrappers actualicen estados y disparen eventos.
    // En la versión síncrona el cliente es responsable de llamar a poll()
    void pollJobState(const JobId& id) {
        JobState state = m_backend->getJobState(id);
        if (state == JobState::DONE && when_job_finished) {
            when_job_finished(id);
        } else if (state == JobState::ERROR && when_job_error) {
            when_job_error(id, "Error procesando el job.");
        }
    }
};

} // namespace horizon::print
