#pragma once
#include <string>
#include <unordered_map>

namespace horizon {

/**
 * @typedef Params
 * @brief Key-value map for translation variables.
 */
using Params = std::unordered_map<std::string, std::string>;

/**
 * @class I18nBackend
 * @brief Abstract interface for internationalization backends.
 * 
 * Thread-safety: Implementations must ensure that translate() is thread-safe for 
 * concurrent reads as long as no mutation (load_locale, set_locale) is occurring.
 */
class I18nBackend {
public:
    virtual ~I18nBackend() = default;

    /**
     * @brief Loads translation data for a specific locale.
     * @param locale The locale identifier (e.g., "en_US", "es").
     * @param path Path to the translation resource.
     * @return true if loaded successfully.
     */
    virtual bool load_locale(const std::string& locale, const std::string& path) = 0;

    /**
     * @brief Sets the active locale for translations.
     * @param locale The locale identifier.
     */
    virtual void set_locale(const std::string& locale) = 0;

    /**
     * @brief Returns the currently active locale.
     */
    virtual std::string get_current_locale() const = 0;

    /**
     * @brief Checks if a translation key exists in the current locale.
     */
    virtual bool has_key(const std::string& key) const = 0;

    /**
     * @brief Translates a key with optional pluralization and variable interpolation.
     * @param key Hierarchical key (e.g., "file.open").
     * @param count Pluralization count. If < 0, pluralization is disabled.
     * @param vars Map of variables to interpolate into the string.
     * @return The translated string, or the original key if not found.
     */
    virtual std::string translate(
        const std::string& key, 
        int count, 
        const Params& vars
    ) const = 0;
};

} // namespace horizon
