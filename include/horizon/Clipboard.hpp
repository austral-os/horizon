#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>


namespace horizon {

/**
 * @class ClipboardData
 * @brief Holds MIME-mapped binary data for clipboard operations.
 */
class ClipboardData {
public:
    void set(const std::string& mime, const std::vector<uint8_t>& data);
    bool has(const std::string& mime) const;
    std::vector<uint8_t> get(const std::string& mime) const;
    std::vector<std::string> mime_types() const;

    bool has_text() const {
        return has("text/plain") || has("text/plain;charset=utf-8");
    }


private:
    std::unordered_map<std::string, std::vector<uint8_t>> m_data;
};

/**
 * @class Clipboard
 * @brief Static interface for system clipboard operations.
 */
class Clipboard {
public:
    /**
     * @brief Sets the system clipboard selection.
     * @param data The data to set.
     */
    static void set(const ClipboardData& data);

    /**
     * @param text The text to set.
     */
    static void set_text(const std::string& text);

    /**
     * @brief Gets the current system clipboard content.
     * @return Optional ClipboardData if available.
     */
    static std::optional<ClipboardData> get();

    /**
     * @brief Helper to get plain text from the clipboard.
     * @return Optional string containing the text.
     */
    static std::optional<std::string> get_text();
};


} // namespace horizon
