#include "horizon/I18n.hpp"
#include "horizon/JsonBackend.hpp"
#include <iostream>
#include <iomanip>

void print_tr(const std::string& key, int count = -1, const horizon::Params& vars = {}) {
    std::string result = (count >= 0) 
        ? horizon::i18n().tr(key, count, vars)
        : horizon::i18n().tr(key, vars);
        
    std::cout << std::left << std::setw(30) << key 
              << " | count=" << std::setw(2) << count 
              << " | result: " << result << std::endl;
}

int main() {
    // 1. Setup Backend
    auto backend = std::make_unique<horizon::JsonBackend>();
    
    // 2. Load locales
    backend->load_locale("es", "examples/i18n/es.json");
    backend->load_locale("en", "examples/i18n/en.json");
    
    // Load newly created core locales
    backend->load_locale("es", "share/locales/core_es.json");
    backend->load_locale("en", "share/locales/core_en.json");

    // 3. Initialize Global I18n
    horizon::i18n().set_backend(std::move(backend));

    std::cout << "--- Testing Locale: Spanish ---" << std::endl;
    horizon::i18n().set_locale("es");
    print_tr("file.open");
    print_tr("file.count", 1);
    print_tr("file.count", 5);
    print_tr("core.dialog.accept");
    print_tr("core.dialog.cancel");
    print_tr("core.search.placeholder");
    print_tr("core.global_menu.edit");

    std::cout << "\n--- Testing Locale: English ---" << std::endl;
    horizon::i18n().set_locale("en");
    print_tr("file.open");
    print_tr("file.count", 1);
    print_tr("file.count", 5);
    print_tr("core.dialog.accept");
    print_tr("core.dialog.cancel");
    print_tr("core.search.placeholder");
    print_tr("core.global_menu.edit");

    return 0;
}
