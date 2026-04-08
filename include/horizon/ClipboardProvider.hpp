#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace horizon {

/**
 * @class DataSink
 * @brief Abstract interface for receiving data asynchronously.
 */
class DataSink {
public:
    virtual ~DataSink() = default;
    
    /**
     * @brief Writes data to the sink.
     */
    virtual void write(const std::vector<uint8_t>& data) = 0;
    
    /**
     * @brief Signals that the transfer is complete.
     */
    virtual void done() {}
    
    /**
     * @brief Signals that an error occurred during transfer.
     */
    virtual void error() {}
};

/**
 * @class ClipboardProvider
 * @brief Abstract interface for providing clipboard data on demand.
 */
class ClipboardProvider {
public:
    virtual ~ClipboardProvider() = default;

    /**
     * @brief Called when another application requests data.
     * @param mime The requested MIME type.
     * @param sink The sink to write the data into.
     */
    virtual void provide_clipboard_data(const std::string& mime, DataSink& sink) = 0;

    /**
     * @brief Returns the list of MIME types provided by this object.
     */
    virtual std::vector<std::string> provided_mime_types() const = 0;
};

} // namespace horizon
