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

    static std::string urlDecode(const std::string& str) {
        std::string ret;
        char ch;
        int i, ii;
        for (i=0; i<str.length(); i++) {
            if (str[i] != '%') {
                if(str[i] == '+')
                    ret += ' ';
                else
                    ret += str[i];
            } else {
                sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
                ch = static_cast<char>(ii);
                ret += ch;
                i = i + 2;
            }
        }
        return ret;
    }

    void scanLoop() {
        FILE* pipe = popen("/usr/sbin/lpinfo -v", "r");
        if (pipe) {
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                if (!m_scanning) break;
                
                std::string line(buffer);
                if (!line.empty() && line.back() == '\n') line.pop_back();

                if (line.find("network ") == 0) {
                    std::string uri = line.substr(8);
                    
                    // Solo agregamos URIs que parezcan endpoints concretos
                    if (uri.find("://") != std::string::npos) {
                        Printer p;
                        p.uri = uri;
                        p.source = PrinterSource::Discovered;
                        
                        // Extraer un nombre amigable del URI
                        std::string name = uri;
                        auto scheme_pos = name.find("://");
                        if (scheme_pos != std::string::npos) {
                            name = name.substr(scheme_pos + 3);
                            auto slash_pos = name.find("/");
                            if (slash_pos != std::string::npos) {
                                name = name.substr(0, slash_pos);
                            }
                            auto dot_pos = name.find("._");
                            if (dot_pos != std::string::npos) {
                                name = name.substr(0, dot_pos);
                            }
                            name = urlDecode(name);
                        }
                        
                        p.name = name;
                        p.id = name; // Guardamos el nombre decodeado como ID base
                        
                        // Limpiar ID para que sea seguro
                        for (char& c : p.id) {
                            if (c == ' ' || c == '/' || c == '#') c = '_';
                        }
                        
                        if (when_printer_found) {
                            when_printer_found(p);
                        }
                    }
                }
            }
            pclose(pipe);
        }

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
