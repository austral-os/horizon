#include "horizon/I18n.hpp"
#include "horizon/JsonBackend.hpp"
#include <cstdlib>
#include <unistd.h>
#include <limits.h>
#include <algorithm>
#include <dirent.h>
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

bool I18n::load_app_locale(const std::string& app_id, const std::string& locale) {
    LOG_INFO << "I18n: Loading app locale '" << locale << "' for app: " << app_id;

    // Build fallback chain: requested locale -> global locale chain -> en
    std::vector<std::string> locales;

    // 1. Requested locale with its own chain (e.g., "es_AR" -> ["es_AR", "es"])
    auto requested_chain = resolve_locale_chain(locale);
    for (const auto& l : requested_chain) {
        if (std::find(locales.begin(), locales.end(), l) == locales.end()) {
            locales.push_back(l);
        }
    }

    // 2. Global locale chain as secondary fallback (if different from requested)
    if (locale != m_current_locale) {
        auto global_chain = resolve_locale_chain(m_current_locale);
        for (const auto& l : global_chain) {
            if (std::find(locales.begin(), locales.end(), l) == locales.end()) {
                locales.push_back(l);
            }
        }
    }

    // 3. Always fall back to English
    if (std::find(locales.begin(), locales.end(), "en") == locales.end()) {
        locales.push_back("en");
    }

    bool any_loaded = false;
    bool requested_loaded = false;
    for (const auto& loc : locales) {
        bool loaded = false;
        for (const auto& base_path : s_search_paths) {
            std::vector<std::string> candidates = {
                base_path + "/apps/" + app_id + "/locales/" + loc + ".json",
                base_path + "/libs/" + app_id + "/locales/" + loc + ".json",
                base_path + "/" + app_id + "/locales/" + loc + ".json",
                base_path + "/locales/" + loc + ".json",
            };

            for (const auto& path : candidates) {
                if (load_locale(loc, path)) {
                    loaded = true;
                    any_loaded = true;
                    if (std::find(requested_chain.begin(), requested_chain.end(), loc)
                        != requested_chain.end()) {
                        requested_loaded = true;
                    }
                    break;
                }
            }
            if (loaded) break;
        }
    }

    // Only switch locale and return true if the requested locale chain
    // itself (e.g. "es_AR" -> "es") actually had translation files.
    // If only fallback locales (global, en) loaded, return false so
    // callers can fall back to the global default instead of setting
    // an invalid current locale.
    if (requested_loaded) {
        // Propagate the global locale to the backend so translate() can
        // fall back through it when the app locale doesn't have a key.
        auto* jb = dynamic_cast<JsonBackend*>(m_backend.get());
        if (jb && !m_global_locale.empty()) {
            jb->set_global_locale(m_global_locale);
        }
        // Switch the current locale WITHOUT redefining the global fallback
        set_current_locale(locale);
    }

    return requested_loaded;
}

std::vector<std::string> I18n::available_app_locales(const std::string& app_id) const {
    std::vector<std::string> result;
    for (const auto& base_path : s_search_paths) {
        // Scan only the app-specific locale paths that load_app_locale()
        // searches.  Do NOT scan the generic locales/ directory: its files
        // are core/system locales, not per-app locale files, and including
        // them would expose core locales as app options.
        std::vector<std::string> dir_candidates = {
            base_path + "/apps/" + app_id + "/locales/",
            base_path + "/libs/" + app_id + "/locales/",
            base_path + "/" + app_id + "/locales/",
        };

        for (const auto& dir_path : dir_candidates) {
            DIR* d = opendir(dir_path.c_str());
            if (!d) continue;
            struct dirent* entry;
            while ((entry = readdir(d)) != nullptr) {
                std::string name(entry->d_name);
                // Match *.json files, exclude hidden/dotfiles
                if (name.size() > 5 && name.substr(name.size() - 5) == ".json" && name[0] != '.') {
                    std::string code = name.substr(0, name.size() - 5);
                    if (std::find(result.begin(), result.end(), code) == result.end()) {
                        result.push_back(code);
                    }
                }
            }
            closedir(d);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::string I18n::get_app_locale_display_name(const std::string& app_id, const std::string& locale) const {
    // Search all app-specific file paths that load_app_locale() searches,
    // keeping display-name resolution consistent with availability.
    for (const auto& base_path : s_search_paths) {
        std::vector<std::string> candidate_paths = {
            base_path + "/apps/" + app_id + "/locales/" + locale + ".json",
            base_path + "/libs/" + app_id + "/locales/" + locale + ".json",
            base_path + "/" + app_id + "/locales/" + locale + ".json",
        };

        for (const auto& path : candidate_paths) {
            std::ifstream f(path);
            if (f.is_open()) {
                try {
                    nlohmann::json data;
                    f >> data;
                    if (data.contains("language") && data["language"].contains("local_name")) {
                        return data["language"]["local_name"].get<std::string>();
                    }
                    // Fallback to generic "name" field
                    if (data.contains("language") && data["language"].contains("name")) {
                        return data["language"]["name"].get<std::string>();
                    }
                } catch (...) {
                    // Ignore parse errors, fall through to next candidate
                }
            }
        }
    }
    return locale;
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
    
    // Public set_locale() establishes the system/global locale:
    // it updates BOTH global and current, and propagates the global
    // fallback to the backend so translate() can use it.
    m_global_locale = clean_locale;
    m_current_locale = clean_locale;
    m_backend->set_locale(clean_locale);
    
    // Propagate global locale to backend for fallback chain resolution
    auto* jb = dynamic_cast<JsonBackend*>(m_backend.get());
    if (jb) {
        jb->set_global_locale(clean_locale);
    }
}

void I18n::set_current_locale(const std::string& locale) {
    if (!m_backend) return;
    
    std::string clean_locale = locale;
    size_t dot = clean_locale.find('.');
    if (dot != std::string::npos) {
        clean_locale = clean_locale.substr(0, dot);
    }
    
    // Internal-only: switch the active locale without touching
    // the global fallback. The caller (e.g. load_app_locale())
    // is responsible for propagating m_global_locale to the backend
    // separately.
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
