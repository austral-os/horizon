#pragma once

#include <horizon/print/IPrintBackend.h>
#include <string>
#include <vector>

namespace horizon::print::backends {

class CUPSBackend : public IPrintBackend {
public:
    std::vector<Printer> listPrinters() override;
    PrinterId addPrinter(const std::string& name, const std::string& uri, const PrintConfig& config) override;
    void removePrinter(const PrinterId& id) override;
    JobId submitJob(const PrinterId& printerId, const PrintDocument& document, const PrintConfig& config) override;
    void cancelJob(const JobId& jobId) override;
    JobState getJobState(const JobId& jobId) override;
};

} // namespace horizon::print::backends
