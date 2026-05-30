#pragma once

#include "Models.h"
#include <functional>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

#include <cups/cups.h>

namespace horizon::print {

class PrinterDiscovery {
private:
    std::atomic<bool> m_scanning{false};
    std::thread m_scanThread;

    static int cupsEnumDestsCallback(void *user_data, unsigned flags, cups_dest_t *dest) {
        auto* discovery = static_cast<PrinterDiscovery*>(user_data);
        if (!discovery->m_scanning) return 0; // Stop enumeration

        Printer p;
        p.id = dest->name;
        p.name = dest->name;
        
        const char* uri = cupsGetOption("device-uri", dest->num_options, dest->options);
        if (uri) {
            p.uri = uri;
        } else {
            p.uri = "ipp://localhost/printers/" + p.id; // fallback
        }
        
        const char* info = cupsGetOption("printer-info", dest->num_options, dest->options);
        if (info && info[0] != '\0') {
            p.name = info;
        }

        p.source = PrinterSource::Discovered;

        if (discovery->when_printer_found) {
            discovery->when_printer_found(p);
        }

        return 1; // Continue enumeration
    }

    void scanLoop() {
        // 1. Enviar una impresora mock para desarrollo/pruebas
        Printer discoveredPrinter;
        discoveredPrinter.id = "discovered_mock_1";
        discoveredPrinter.name = "Mock Network Printer";
        discoveredPrinter.uri = "ipp://192.168.1.100/ipp/print";
        discoveredPrinter.source = PrinterSource::Discovered;

        if (when_printer_found) {
            when_printer_found(discoveredPrinter);
        }

        // 2. Ejecutar escaneo real vía CUPS (DNS-SD, mDNS, USB, etc.)
        // cupsEnumDests bloquea hasta que termine el tiempo especificado.
        // Usamos un timeout de 5000ms (5 segundos).
        cupsEnumDests(CUPS_DEST_FLAGS_UNCONNECTED | CUPS_DEST_FLAGS_CANCELED, 
                      5000, NULL, 0, 0, cupsEnumDestsCallback, this);

        while (m_scanning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

public:
    // Eventos planos
    std::function<void(const Printer&)> when_printer_found;
    std::function<void(const PrinterId&)> when_printer_lost;

    PrinterDiscovery() = default;
    
    ~PrinterDiscovery() {
        stopScan();
    }

    void startScan() {
        if (m_scanning) return;
        m_scanning = true;
        
        // Ejecutar escaneo en hilo separado
        m_scanThread = std::thread(&PrinterDiscovery::scanLoop, this);
    }

    void stopScan() {
        m_scanning = false;
        // En producción: detener Avahi browser
        if (m_scanThread.joinable()) {
            m_scanThread.join();
        }
    }
    
    bool isScanning() const {
        return m_scanning;
    }
};

} // namespace horizon::print
