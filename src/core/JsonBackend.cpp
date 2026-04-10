#include "horizon/JsonBackend.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <horizon/Logger.hpp>

namespace horizon {

namespace {
    void deep_merge(nlohmann::json& target, const nlohmann::json& source) {
        for (auto it = source.begin(); it != source.end(); ++it) {
            if (it.value().is_object() && target.contains(it.key()) && target[it.key()].is_object()) {
                deep_merge(target[it.key()], it.value());
            } else {
                target[it.key()] = it.value();
            }
        }
    }

    std::vector<std::string> resolve_chain(const std::string& locale) {
        std::vector<std::string> chain;
        if (!locale.empty()) {
            chain.push_back(locale);
            size_t underscore = locale.find('_');
            if (underscore != std::string::npos) {
                chain.push_back(locale.substr(0, underscore));
            }
        }
        // Universal fallback
        if (locale != "en") {
            chain.push_back("en");
        }
        return chain;
    }
}

bool JsonBackend::load_locale(const std::string& locale, const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_INFO << "I18n: Failed to open candidate path: " << path;
        return false;
    }

    try {
        nlohmann::json data;
        file >> data;
        
        if (m_locales.count(locale)) {
            deep_merge(m_locales[locale], data);
        } else {
            m_locales[locale] = std::move(data);
        }
        
        LOG_INFO << "I18n: Successfully loaded locale '" << locale << "' from " << path;

        // If it's the first locale loaded, set it as current
        if (m_current_locale.empty()) {
            m_current_locale = locale;
        }
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        // In a production framework, we might use Logger here
        std::cerr << "I18n: Failed to parse JSON from " << path << ": " << e.what() << std::endl;
        return false;
    }
}

void JsonBackend::set_locale(const std::string& locale) {
    m_current_locale = locale;
}

bool JsonBackend::has_key(const std::string& key) const {
    return find_node(key) != nullptr;
}

std::string JsonBackend::translate(const std::string& key, int count, const Params& vars) const {
    auto chain = resolve_chain(m_current_locale);
    const nlohmann::json* node = nullptr;

    for (const auto& loc : chain) {
        node = find_node_in_locale(loc, key);
        if (node) break;
    }

    if (!node) {
        LOG_INFO << "I18n: Translation key not found: " << key << " (Locale: " << m_current_locale << ")";
        return key;
    }

    std::string result;

    if (count >= 0 && node->is_object()) {
        // Pluralization logic
        std::string plural_key = (count == 1) ? "one" : "other";
        
        if (node->contains(plural_key)) {
            const auto& plural_node = (*node)[plural_key];
            if (plural_node.is_string()) {
                result = plural_node.get<std::string>();
            } else {
                return key; // Plural leaf is not a string
            }
        } else if (plural_key == "one" && node->contains("other")) {
            // Fallback: missing 'one' uses 'other'
            const auto& other_node = (*node)["other"];
            if (other_node.is_string()) {
                result = other_node.get<std::string>();
            } else {
                return key;
            }
        } else {
            return key; // Plural key not found
        }

        // Interpolate with {count}
        Params final_vars = vars;
        final_vars["count"] = std::to_string(count);
        return interpolate(result, final_vars);
    } else if (node->is_string()) {
        // Simple translation
        result = node->get<std::string>();
        return interpolate(result, vars);
    }

    return key; // Node exists but type is invalid for translation
}

std::string JsonBackend::interpolate(const std::string& input, const Params& vars) const {
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '{') {
            size_t end = input.find('}', i);
            if (end != std::string::npos) {
                std::string var_name = input.substr(i + 1, end - i - 1);
                auto it = vars.find(var_name);
                if (it != vars.end()) {
                    result += it->second;
                } else {
                    // Variable not found, leave placeholder
                    result += "{" + var_name + "}";
                }
                i = end;
                continue;
            }
        }
        result += input[i];
    }

    return result;
}

const nlohmann::json* JsonBackend::find_node(const std::string& key) const {
    auto chain = resolve_chain(m_current_locale);
    for (const auto& loc : chain) {
        const nlohmann::json* node = find_node_in_locale(loc, key);
        if (node) return node;
    }
    return nullptr;
}

const nlohmann::json* JsonBackend::find_node_in_locale(const std::string& locale, const std::string& key) const {
    auto it = m_locales.find(locale);
    if (it == m_locales.end()) {
        return nullptr;
    }

    const nlohmann::json* current = &it->second;
    
    std::stringstream ss(key);
    std::string segment;
    while (std::getline(ss, segment, '.')) {
        if (current->is_object() && current->contains(segment)) {
            current = &(*current)[segment];
        } else {
            return nullptr;
        }
    }

    return current;
}

} // namespace horizon
