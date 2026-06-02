#include "horizon/I18n.hpp"
#include "horizon/JsonBackend.hpp"
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include <horizon/Logger.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

namespace horizon {

I18n::I18n(std::unique_ptr<I18nBackend> backend) 
    : m_backend(std::move(backend)) {}

std::vector<std::string> I18n::s_search_paths = {};

void I18n::add_search_path(const std::string& path) {
    s_search_paths.push_back(path);
}

void I18n::set_search_paths(const std::vector<std::string>& paths) {
    s_search_paths = paths;
}

bool I18n::load_core_locales() {
    LOG_INFO << "I18n: Loading core system locales...";
    auto locales = resolve_locale_chain(m_current_locale);
    if (std::find(locales.begin(), locales.end(), "en") == locales.end()) {
        locales.push_back("en");
    }

    bool any_loaded = false;

    for (const auto& locale : locales) {
        bool loaded = false;
        for (const auto& base_path : s_search_paths) {
            std::string path = base_path + "/locales/core_" + locale + ".json";
            if (load_locale(locale, path)) {
                loaded = true;
                any_loaded = true;
                break;
            }
        }
    }
    return any_loaded;
}

bool I18n::load_app_locales(const std::string& app_id) {
    LOG_INFO << "I18n: Loading locales for app: " << app_id;
    auto locales = resolve_locale_chain(m_current_locale);
    if (std::find(locales.begin(), locales.end(), "en") == locales.end()) {
        locales.push_back("en");
    }

    bool any_loaded = false;

    for (const auto& locale : locales) {
        bool loaded = false;
        for (const auto& base_path : s_search_paths) {
            std::vector<std::string> candidates = {
                base_path + "/apps/" + app_id + "/locales/" + locale + ".json",
                base_path + "/libs/" + app_id + "/locales/" + locale + ".json",
                base_path + "/" + app_id + "/locales/" + locale + ".json",
                base_path + "/locales/" + locale + ".json"
            };

            for (const auto& path : candidates) {
                if (load_locale(locale, path)) {
                    loaded = true;
                    any_loaded = true;
                    break;
                }
            }
            if (loaded) break;
        }
    }
    return any_loaded;
}

void I18n::set_backend(std::unique_ptr<I18nBackend> backend) {
    m_backend = std::move(backend);
}

bool I18n::load_locale(const std::string& locale, const std::string& path) {
    if (!m_backend) return false;
    return m_backend->load_locale(locale, path);
}

void I18n::set_locale(const std::string& locale) {
    if (!m_backend) return;
    
    std::string clean_locale = locale;
    size_t dot = clean_locale.find('.');
    if (dot != std::string::npos) {
        clean_locale = clean_locale.substr(0, dot);
    }
    
    m_current_locale = clean_locale;
    m_backend->set_locale(clean_locale);
}

std::string I18n::tr(const std::string& key, const Params& vars) const {
    if (!m_backend) return key;
    return m_backend->translate(key, -1, vars);
}

std::string I18n::tr(const std::string& key, int count, const Params& vars) const {
    if (!m_backend) return key;
    return m_backend->translate(key, count, vars);
}

std::vector<std::string> I18n::resolve_locale_chain(const std::string& locale) {
    std::vector<std::string> chain;
    chain.push_back(locale);
    
    size_t underscore = locale.find('_');
    if (underscore != std::string::npos) {
        chain.push_back(locale.substr(0, underscore));
    }
    
    // Potential for more logic here (e.g. "es-419" -> ["es-419", "es"])
    return chain;
}

std::string I18n::get_language_name(const std::string& code) const {
    for (const auto& base_path : s_search_paths) {
        std::vector<std::string> candidates = {
            base_path + "/apps/horizon-installer/locales/" + code + ".json",
            base_path + "/locales/" + code + ".json"
        };

        for (const auto& path : candidates) {
            if (access(path.c_str(), F_OK) == 0) {
                try {
                    std::ifstream f(path);
                    if (!f.is_open()) continue;
                    nlohmann::json data;
                    f >> data;
                    if (data.contains("language") && data["language"].contains("name")) {
                        return data["language"]["name"].get<std::string>();
                    }
                } catch (...) {
                    continue;
                }
            }
        }
    }
    return code;
}

std::string I18n::get_valid_system_locale(const std::string& lang_code, const std::string& country_code) {
    std::string upper_country = country_code;
    for (auto & c: upper_country) c = toupper(c);
    
    std::string target_locale = lang_code + "_" + upper_country + ".UTF-8";
    
    std::ifstream supported_file("/usr/share/i18n/SUPPORTED");
    if (!supported_file.is_open()) {
        return target_locale;
    }

    std::string line;
    bool found_target = false;
    std::string fallback_locale = "";
    std::string lang_prefix = lang_code + "_";
    
    while (std::getline(supported_file, line)) {
        if (line.find(target_locale) != std::string::npos) {
            found_target = true;
            break;
        }
        if (fallback_locale.empty() && line.find(lang_prefix) == 0 && line.find(".UTF-8") != std::string::npos) {
            size_t space_pos = line.find(' ');
            if (space_pos != std::string::npos) {
                fallback_locale = line.substr(0, space_pos);
            }
        }
    }

    if (found_target) {
        return target_locale;
    }
    
    if (!fallback_locale.empty()) {
        LOG_WARNING << "Locale " << target_locale << " not supported. Falling back to " << fallback_locale;
        return fallback_locale;
    }
    
    return target_locale;
}

I18n& i18n() {
    static I18n instance;
    if (!instance.get_backend()) {
        instance.set_backend(std::make_unique<JsonBackend>());
        
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            LOG_INFO << "I18n: Current working directory: " << cwd;
        }

        // Default search paths
        I18n::add_search_path(".");
#ifdef HORIZON_SOURCE_DIR
        I18n::add_search_path(HORIZON_SOURCE_DIR);
        I18n::add_search_path(std::string(HORIZON_SOURCE_DIR) + "/share");
#endif
        I18n::add_search_path("/usr/share/horizon");
        
        // Automatic locale detection from environment
        const char* lang = std::getenv("LANG");
        if (lang) {
            std::string lang_str(lang);
            // lang is often like "es_ES.UTF-8", we want "es_ES" or "es"
            size_t dot = lang_str.find('.');
            if (dot != std::string::npos) {
                lang_str = lang_str.substr(0, dot);
            }
            instance.set_locale(lang_str);
        } else {
            instance.set_locale("en");
        }

        // Always attempt to load core system locales upon initialization
        instance.load_core_locales();
    }
    return instance;
}

} // namespace horizon
