#pragma once

#include "horizon/EventsManager.hpp"
#include <string>

namespace horizon::files
{
    /**
     * @struct OperationProgressEvent
     * @brief Represents the progress of a file operation.
     */
    struct OperationProgressEvent : public EventContext
    {
        double progress; // 0.0 to 1.0
        bool finished = false;
    };

    /**
     * @struct PathChangedEvent
     * @brief Emitted when the current path changes.
     */
    struct PathChangedEvent : public EventContext
    {
        std::string path;
    };
} // namespace horizon::files
