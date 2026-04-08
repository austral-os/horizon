#include "horizon/Clipboard.hpp"
#include "WaylandClipboardBackend.hpp"
#include "horizon/WaylandSurface.hpp"
#include "horizon/WaylandWindow.hpp"
#include "horizon/Logger.hpp"
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <errno.h>

namespace horizon {

// --- ClipboardData Implementation ---

void ClipboardData::set(const std::string& mime, const std::vector<uint8_t>& data) {
    m_data[mime] = data;
}

bool ClipboardData::has(const std::string& mime) const {
    return m_data.count(mime) > 0;
}

std::vector<uint8_t> ClipboardData::get(const std::string& mime) const {
    auto it = m_data.find(mime);
    if (it != m_data.end()) return it->second;
    return {};
}

std::vector<std::string> ClipboardData::mime_types() const {
    std::vector<std::string> types;
    for (const auto& [mime, _] : m_data) types.push_back(mime);
    return types;
}

// --- Clipboard Implementation ---

void Clipboard::set(const ClipboardData& data) {
    auto* window = WaylandWindow::get_active_window();
    if (window) {
        window->set_clipboard_data(data);
    }
}

void Clipboard::set_text(const std::string& text) {
    ClipboardData data;
    std::vector<uint8_t> bytes(text.begin(), text.end());
    data.set("text/plain", bytes);
    data.set("text/plain;charset=utf-8", bytes);
    set(data);
}

} // namespace horizon
