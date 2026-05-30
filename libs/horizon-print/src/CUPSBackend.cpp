#include <horizon/print/backends/CUPSBackend.h>
#include <horizon/print/backends/IPPMapper.h>
#include <cups/cups.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdlib.h>

namespace horizon::print::backends {

std::vector<Printer> CUPSBackend::listPrinters() {
    std::vector<Printer> printers;
    cups_dest_t *dests;
    int num_dests = cupsGetDests(&dests);

    for (int i = 0; i < num_dests; ++i) {
        cups_dest_t dest = dests[i];
        Printer p;
        p.id = dest.name;
        p.name = dest.name;
        
        // Retrieve URI if possible
        const char* uri = cupsGetOption("device-uri", dest.num_options, dest.options);
        if (uri) {
            p.uri = uri;
        } else {
            p.uri = "unknown";
        }
        
        p.source = PrinterSource::Installed;
        printers.push_back(p);
    }

    cupsFreeDests(num_dests, dests);
    return printers;
}

PrinterId CUPSBackend::addPrinter(const std::string& name, const std::string& uri, const PrintConfig& config) {
    // CUPS no permite espacios en el ID de la impresora (-p)
    std::string safe_id = name;
    for (char& c : safe_id) {
        if (c == ' ' || c == '/' || c == '#') c = '_';
    }

    // Combinamos ambos intentos en un solo script bash ejecutado vía pkexec.
    // Esto evita que Polkit pida la contraseña dos veces.
    // Además, usamos `timeout 5` para evitar que lpadmin se cuelgue 60 segundos si 
    // la impresora no soporta IPP Everywhere (muy común en impresoras LPD/antiguas).
    std::string script = 
        "if ! timeout 5 /usr/sbin/lpadmin -p '" + safe_id + "' -v '" + uri + "' -m everywhere -E 2>/dev/null; then "
        "  /usr/sbin/lpadmin -p '" + safe_id + "' -v '" + uri + "' -E; "
        "fi";
    
    std::string cmd = "pkexec bash -c \"" + script + "\"";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        throw std::runtime_error("No se pudo agregar la impresora. Asegúrate de tener permisos (polkit/sudo).");
    }
    
    return safe_id;
}

void CUPSBackend::removePrinter(const PrinterId& id) {
    std::string cmd = "pkexec /usr/sbin/lpadmin -x \"" + id + "\"";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        throw std::runtime_error("No se pudo eliminar la impresora.");
    }
}

JobId CUPSBackend::submitJob(const PrinterId& printerId, const PrintDocument& document, const PrintConfig& config) {
    if (!document.isValid()) {
        throw std::invalid_argument("Documento PDF inválido o corrupto");
    }

    // Escribir a un archivo temporal
    char tmpFile[] = "/tmp/horizon_print_XXXXXX.pdf";
    int fd = mkstemps(tmpFile, 4);
    if (fd == -1) {
        throw std::runtime_error("No se pudo crear archivo temporal para impresión.");
    }
    
    if (write(fd, document.data.data(), document.data.size()) == -1) {
        close(fd);
        unlink(tmpFile);
        throw std::runtime_error("Error escribiendo archivo temporal de impresión.");
    }
    close(fd);

    auto options = IPPMapper::toCUPSOptions(config);
    
    int num_options = 0;
    cups_option_t *cups_options = nullptr;
    for (const auto& [k, v] : options) {
        num_options = cupsAddOption(k.c_str(), v.c_str(), num_options, &cups_options);
    }

    int jobId = cupsPrintFile(printerId.c_str(), tmpFile, "Horizon Print Job", num_options, cups_options);
    
    cupsFreeOptions(num_options, cups_options);
    unlink(tmpFile);

    if (jobId == 0) {
        throw std::runtime_error(cupsLastErrorString());
    }

    return std::to_string(jobId);
}

void CUPSBackend::cancelJob(const JobId& jobId) {
    // cupsCancelJob2 requiere CUPS 1.4+. cupsCancelJob(dest, id) 
    // Como no tenemos el dest_name fácil, usamos CUPS_HTTP_DEFAULT
    cupsCancelJob2(CUPS_HTTP_DEFAULT, nullptr, std::stoi(jobId), 1);
}

JobState CUPSBackend::getJobState(const JobId& jobId) {
    cups_job_t *jobs;
    // Buscamos trabajos de todos los usuarios
    int num_jobs = cupsGetJobs(&jobs, nullptr, 0, CUPS_WHICHJOBS_ALL);
    int targetId = std::stoi(jobId);
    
    JobState state = JobState::DONE; // Asumimos DONE si desaparece (CUPS los limpia rápido a veces)
    
    for (int i = 0; i < num_jobs; ++i) {
        if (jobs[i].id == targetId) {
            switch(jobs[i].state) {
                case IPP_JOB_PENDING:
                case IPP_JOB_HELD:
                    state = JobState::QUEUED;
                    break;
                case IPP_JOB_PROCESSING:
                    state = JobState::RUNNING;
                    break;
                case IPP_JOB_STOPPED:
                case IPP_JOB_CANCELED:
                case IPP_JOB_ABORTED:
                    state = JobState::ERROR;
                    break;
                case IPP_JOB_COMPLETED:
                    state = JobState::DONE;
                    break;
            }
            break;
        }
    }
    
    cupsFreeJobs(num_jobs, jobs);
    return state;
}

} // namespace horizon::print::backends
