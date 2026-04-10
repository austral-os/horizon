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

private:
    std::string interpolate(const std::string& input, const Params& vars) const;
    const nlohmann::json* find_node(const std::string& key) const;
    const nlohmann::json* find_node_in_locale(const std::string& locale, const std::string& key) const;

    std::map<std::string, nlohmann::json> m_locales;
    std::string m_current_locale;
};

} // namespace horizon
