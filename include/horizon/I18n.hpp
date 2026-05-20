#pragma once
#include <memory>
#include <string>
#include <vector>
#include "horizon/I18nBackend.hpp"

namespace horizon {

/**
 * @class I18n
 * @brief Main entry point for the Horizon internationalization system.
 * 
 * Thread-safety: 
 * - Mutation (set_backend, set_locale, load_locale) is NOT thread-safe.
 * - tr() is safe FOR CONCURRENT READS only if no concurrent mutation occurs.
 */
class I18n {
public:
    I18n() = default;
    explicit I18n(std::unique_ptr<I18nBackend> backend);

    // Disable copying
    I18n(const I18n&) = delete;
    I18n& operator=(const I18n&) = delete;
    
    // Allow moving
    I18n(I18n&&) = default;
    I18n& operator=(I18n&&) = default;

    /**
     * @brief Replaces the current backend.
     */
    void set_backend(std::unique_ptr<I18nBackend> backend);

    /**
     * @brief Access the current backend. Returns nullptr if none is set.
     */
    const I18nBackend* get_backend() const { return m_backend.get(); }

    /**
     * @brief Loads a locale via the active backend.
     */
    bool load_locale(const std::string& locale, const std::string& path);

    /**
     * @brief Sets the active locale via the active backend.
     */
    void set_locale(const std::string& locale);

    /**
     * @brief Translates a key with optional variables.
     */
    std::string tr(const std::string& key, const Params& vars = {}) const;

    /**
     * @brief Translates a key with pluralization and optional variables.
     */
    std::string tr(const std::string& key, int count, const Params& vars = {}) const;

    /**
     * @brief Adds a directory to the locale search path.
     */
    static void add_search_path(const std::string& path);

    /**
     * @brief Sets the entire locale search path.
     */
    static void set_search_paths(const std::vector<std::string>& paths);

    /**
     * @brief Returns the current locale search paths.
     */
    static const std::vector<std::string>& get_search_paths() { return s_search_paths; }

    /**
     * @brief Automatically discovers and loads core localization files (share/locales/core_*.json).
     * @return true if at least one core locale was loaded.
     */
    bool load_core_locales();

    /**
     * @brief Loads application-specific locales by searching standard paths.
     * Searches for 'locales/[locale].json' relative to search paths.
     */
    bool load_app_locales(const std::string& app_id);

    /**
     * @brief Resolves a locale chain (e.g., "es_AR" -> ["es_AR", "es"]).
     */
    static std::vector<std::string> resolve_locale_chain(const std::string& locale);

    /**
     * @brief Returns the human-readable name of a language code (e.g. "es" -> "Español").
     * Searches through available locale files for metadata.
     */
    std::string get_language_name(const std::string& code) const;

    /**
     * @brief Checks if a locale is supported by the system and returns a valid fallback if not.
     */
    static std::string get_valid_system_locale(const std::string& lang_code, const std::string& country_code);


private:
    static std::vector<std::string> s_search_paths;
    std::unique_ptr<I18nBackend> m_backend;
    std::string m_current_locale;
};

/**
 * @brief Global accessor for the primary I18n instance.
 * The instance is lazy-initialized. A backend MUST be set before use.
 */
I18n& i18n();

} // namespace horizon
