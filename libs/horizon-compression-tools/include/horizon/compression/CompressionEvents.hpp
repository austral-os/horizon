#pragma once

#include <horizon/EventsManager.hpp>
#include <string>

namespace horizon::compression
{
    /**
     * @brief Event context for compression/decompression progress.
     */
    class CompressionProgressEvent : public EventContext
    {
    public:
        double progress{0.0}; // 0.0 to 1.0
        std::string current_file;
        std::string status_message;
    };

    /**
     * @brief Event context for completion of compression/decompression tasks.
     */
    class CompressionFinishedEvent : public EventContext
    {
    public:
        bool success{false};
        std::string error_message;
        std::string output_path;
    };

} // namespace horizon::compression
