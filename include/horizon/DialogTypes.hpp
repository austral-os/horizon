#pragma once

namespace horizon
{
    enum class MessageType
    {
        Info,
        Warning,
        Error,
        Question
    };

    enum class MessageResponse
    {
        Accept,
        Cancel
    };
}
