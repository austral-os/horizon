#pragma once

#include <vector>
#include <string>
#include <memory>
#include "horizon/ClipboardProvider.hpp"
#include "horizon/ClipboardState.hpp"

namespace horizon {

/**
 * @class ClipboardBackend
 * @brief Abstract interface for the low-level clipboard protocol (Wayland, etc.).
 */
class ClipboardBackend {
public:
    virtual ~ClipboardBackend() = default;

    /**
     * @brief Sets a local provider as the owner of the clipboard selection.
     */
    virtual void set_provider(
        ClipboardProvider* provider,
        const std::vector<std::string>& mime_types
    ) = 0;

    /**
     * @brief Clears the current local provider.
     */
    virtual void clear_provider() = 0;

    /**
     * @brief Returns the MIME types available in the current system clipboard.
     */
    virtual std::vector<std::string> get_mime_types() const = 0;

    /**
     * @brief Requests data for a specific MIME type from the system clipboard.
     */
    virtual void request_data(
        const std::string& mime,
        std::shared_ptr<DataSink> sink
    ) = 0;

    /**
     * @brief Returns the current state of the selection.
     */
    virtual SelectionState get_state() const = 0;

    /**
     * @brief Returns the current generation ID of the selection.
     */
    virtual uint64_t get_current_generation() const = 0;

    /**
     * @brief Notifies the backend that a widget is being destroyed.
     */
    virtual void on_widget_destroyed(class Widget* widget) = 0;
};

} // namespace horizon
