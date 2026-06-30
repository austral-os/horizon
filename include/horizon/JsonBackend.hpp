#pragma once
#include "horizon/I18nBackend.hpp"
#include <nlohmann/json.hpp>
#include <map>

namespace horizon {

/**
 * @class JsonBackend
 * @brief I18nBackend implementation that uses JSON files.
 */
class JsonBackend : public I18nBackend {
public:
    JsonBackend() = default;

    bool load_locale(const std::string& locale, const std::string& path) override;
    void set_locale(const std::string& locale) override;
    std::string get_current_locale() const override { return m_current_locale; }
    
    bool has_key(const std::string& key) const override;

    std::string translate(
        const std::string& key, 
        int count, 
        const Params& vars
    ) const override;

    /**
     * @brief Sets the global (system) fallback locale for the fallback chain.
     * When the current locale doesn't have a key, the backend falls back
     * through the global locale before trying 'en'.
     */
    void set_global_locale(const std::string& locale) { m_global_locale = locale; }

    /**
     * @brief Returns the current global fallback locale.
     */
    std::string get_global_locale() const { return m_global_locale; }

private:
    std::string interpolate(const std::string& input, const Params& vars) const;
    const nlohmann::json* find_node(const std::string& key) const;
    const nlohmann::json* find_node_in_locale(const std::string& locale, const std::string& key) const;

    std::map<std::string, nlohmann::json> m_locales;
    std::string m_current_locale;
    std::string m_global_locale;
};

} // namespace horizon
