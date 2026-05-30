#pragma once

#include "Models.h"
#include <vector>

namespace horizon::print {

class IPrintBackend {
public:
    virtual ~IPrintBackend() = default;

    // --- Gestión de Impresoras ---
    // Retorna las impresoras instaladas en el sistema (ej. colas CUPS)
    virtual std::vector<Printer> listPrinters() = 0;
    
    // Registra una nueva cola de impresión localmente
    virtual PrinterId addPrinter(const std::string& name, const std::string& uri, const PrintConfig& config) = 0;
    
    // Elimina una cola de impresión
    virtual void removePrinter(const PrinterId& id) = 0;

    // --- Gestión de Trabajos ---
    // Envía un documento a la cola. Devuelve el JobId asignado por el backend.
    virtual JobId submitJob(const PrinterId& printerId, const PrintDocument& document, const PrintConfig& config) = 0;
    
    // Solicita la cancelación de un trabajo en curso o encolado.
    virtual void cancelJob(const JobId& jobId) = 0;

    // Consulta el estado actual de un trabajo directamente al backend.
    virtual JobState getJobState(const JobId& jobId) = 0;
};

} // namespace horizon::print
