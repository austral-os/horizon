#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

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
     * @brief Helper to set plain text to the clipboard.
     * @param text The text to set.
     */
    static void set_text(const std::string& text);
};

} // namespace horizon
