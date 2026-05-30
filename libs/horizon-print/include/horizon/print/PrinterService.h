#pragma once

#include "IPrintBackend.h"
#include <memory>
#include <vector>

namespace horizon::print {

class PrinterService {
private:
    std::shared_ptr<IPrintBackend> m_backend;

public:
    explicit PrinterService(std::shared_ptr<IPrintBackend> backend) : m_backend(std::move(backend)) {}

    std::vector<Printer> listPrinters() const {
        return m_backend->listPrinters();
    }

    PrinterId addPrinter(const std::string& name, const std::string& uri, const PrintConfig& config) {
        return m_backend->addPrinter(name, uri, config);
    }

    void removePrinter(const PrinterId& id) {
        m_backend->removePrinter(id);
    }
};

} // namespace horizon::print
