#include <horizon/print/Models.h>
#include <horizon/print/backends/MockPrintBackend.h>
#include <cassert>
#include <iostream>

using namespace horizon::print;
using namespace horizon::print::backends;

void test_document_validation() {
    PrintDocument validDoc;
    validDoc.data = {'%', 'P', 'D', 'F', '-', '1', '.', '4'};
    assert(validDoc.isValid() && "Valid document should pass validation");

    PrintDocument invalidDoc;
    invalidDoc.data = {'h', 'e', 'l', 'l', 'o'};
    assert(!invalidDoc.isValid() && "Invalid document should fail validation");
    
    std::cout << "[OK] test_document_validation\n";
}

void test_mock_backend_crud() {
    MockPrintBackend backend;
    assert(backend.listPrinters().size() == 0);

    PrintConfig config;
    PrinterId id1 = backend.addPrinter("Test Printer 1", "ipp://localhost/test1", config);
    PrinterId id2 = backend.addPrinter("Test Printer 2", "ipp://localhost/test2", config);

    auto printers = backend.listPrinters();
    assert(printers.size() == 2);
    assert(printers[0].name == "Test Printer 1");
    assert(printers[0].source == PrinterSource::Installed);

    backend.removePrinter(id1);
    assert(backend.listPrinters().size() == 1);
    assert(backend.listPrinters()[0].id == id2);
    
    std::cout << "[OK] test_mock_backend_crud\n";
}

void test_mock_backend_jobs() {
    MockPrintBackend backend;
    PrintConfig config;
    PrinterId pid = backend.addPrinter("Test Printer", "ipp://test", config);

    PrintDocument validDoc;
    validDoc.data = {'%', 'P', 'D', 'F', '-', '1', '.', '4'};

    JobId jobId = backend.submitJob(pid, validDoc, config);
    assert(!jobId.empty());
    assert(backend.getJobState(jobId) == JobState::QUEUED);

    backend.mock_setJobState(jobId, JobState::RUNNING);
    assert(backend.getJobState(jobId) == JobState::RUNNING);
    
    backend.cancelJob(jobId);
    assert(backend.getJobState(jobId) == JobState::ERROR);
    
    std::cout << "[OK] test_mock_backend_jobs\n";
}

void test_mock_backend_rejects_invalid() {
    MockPrintBackend backend;
    PrintConfig config;
    PrinterId pid = backend.addPrinter("Test Printer", "ipp://test", config);

    PrintDocument invalidDoc;
    invalidDoc.data = {'b', 'a', 'd'};

    bool threw = false;
    try {
        backend.submitJob(pid, invalidDoc, config);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw && "Submitting an invalid document should throw std::invalid_argument");
    
    std::cout << "[OK] test_mock_backend_rejects_invalid\n";
}

#include <horizon/print/PrintService.h>
#include <horizon/print/PrinterService.h>

void test_printer_service() {
    auto backend = std::make_shared<MockPrintBackend>();
    PrinterService service(backend);

    assert(service.listPrinters().empty());

    PrintConfig config;
    PrinterId id = service.addPrinter("Test Printer", "ipp://test", config);
    assert(!id.empty());
    assert(service.listPrinters().size() == 1);

    service.removePrinter(id);
    assert(service.listPrinters().empty());

    std::cout << "[OK] test_printer_service\n";
}

void test_print_service() {
    auto backend = std::make_shared<MockPrintBackend>();
    PrintService printService(backend);
    PrinterService printerService(backend);

    PrintConfig config;
    PrinterId pid = printerService.addPrinter("Test Printer", "ipp://test", config);

    bool started = false;
    printService.when_job_started = [&](const JobId& id) { started = true; };

    PrintDocument doc;
    doc.data = {'%', 'P', 'D', 'F', '-', '1', '.', '4'};
    
    JobId jid = printService.submitJob(pid, doc, config);
    assert(!jid.empty());
    assert(started);
    assert(printService.getJobState(jid) == JobState::QUEUED);

    bool finished = false;
    printService.when_job_finished = [&](const JobId& id) { finished = true; };

    backend->mock_setJobState(jid, JobState::DONE);
    printService.pollJobState(jid);
    assert(finished);

    std::cout << "[OK] test_print_service\n";
}

#include <horizon/print/AsyncPrintService.h>

void test_async_service() {
    auto backend = std::make_shared<MockPrintBackend>();
    PrintService printService(backend);
    PrinterService printerService(backend);
    AsyncPrintService asyncService(printService, printerService);

    std::mutex testMutex;
    std::condition_variable testCv;
    bool done = false;
    PrinterId newPid;

    PrintConfig config;
    
    // Test async add printer
    asyncService.addPrinterAsync("Async Printer", "ipp://async", config, [&](PrinterId id) {
        std::lock_guard<std::mutex> lock(testMutex);
        newPid = id;
        done = true;
        testCv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(testMutex);
        testCv.wait(lock, [&]{ return done; });
    }

    assert(!newPid.empty());
    assert(printerService.listPrinters().size() == 1);
    
    // Reset flags
    done = false;
    JobId newJid;

    PrintDocument doc;
    doc.data = {'%', 'P', 'D', 'F', '-', '1', '.', '4'};
    
    // Test async submit job
    asyncService.submitJobAsync(newPid, doc, config, [&](JobId id) {
        std::lock_guard<std::mutex> lock(testMutex);
        newJid = id;
        done = true;
        testCv.notify_one();
    });

    {
        std::unique_lock<std::mutex> lock(testMutex);
        testCv.wait(lock, [&]{ return done; });
    }

    assert(!newJid.empty());
    assert(printService.getJobState(newJid) == JobState::QUEUED);

    std::cout << "[OK] test_async_service\n";
}

#include <horizon/print/backends/IPPMapper.h>

void test_ipp_mapper() {
    PrintConfig config;
    config.copies = 5;
    config.duplex = true;
    config.extra[PrintOption::MediaType] = "photo-glossy";
    config.extra[PrintOption::Resolution] = "600dpi";

    auto mapped = IPPMapper::toCUPSOptions(config);

    assert(mapped["copies"] == "5");
    assert(mapped["sides"] == "two-sided-long-edge");
    assert(mapped["media"] == "photo-glossy");
    assert(mapped["resolution"] == "600dpi");

    std::cout << "[OK] test_ipp_mapper\n";
}

#include <horizon/print/PrinterDiscovery.h>

void test_printer_discovery() {
    PrinterDiscovery discovery;
    
    bool found = false;
    bool lost = false;
    std::mutex mtx;
    std::condition_variable cv;

    discovery.when_printer_found = [&](const Printer& p) {
        std::lock_guard<std::mutex> lock(mtx);
        assert(p.source == PrinterSource::Discovered);
        found = true;
        cv.notify_all();
    };

    discovery.when_printer_lost = [&](const PrinterId& id) {
        std::lock_guard<std::mutex> lock(mtx);
        lost = true;
        cv.notify_all();
    };

    discovery.startScan();
    assert(discovery.isScanning());

    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return found && lost; });
    }

    discovery.stopScan();
    assert(!discovery.isScanning());
    assert(found);
    assert(lost);

    std::cout << "[OK] test_printer_discovery\n";
}

int main() {
    std::cout << "Running horizon-print core tests...\n";
    test_document_validation();
    test_mock_backend_crud();
    test_mock_backend_jobs();
    test_mock_backend_rejects_invalid();
    test_printer_service();
    test_print_service();
    test_async_service();
    test_ipp_mapper();
    test_printer_discovery();
    std::cout << "All tests passed successfully!\n";
    return 0;
}
