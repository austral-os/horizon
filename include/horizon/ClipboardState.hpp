#pragma once

#include <cstdint>

namespace horizon {

class Widget;

/**
 * @enum SelectionState
 * @brief Represents the current state of the clipboard selection.
 */
enum class SelectionState {
    IDLE,           ///< No active selection.
    LOCAL_OWNER,    ///< A widget in this application owns the selection.
    REMOTE_OFFER,   ///< Another application owns the selection.
    TRANSFERRING    ///< Data is currently being transferred.
};

/**
 * @struct SelectionIdentity
 * @brief Uniquely identifies a selection to prevent dangling pointers.
 */
struct SelectionIdentity {
    uint64_t generation_id{0};
    Widget* owner_ptr{nullptr};
};

} // namespace horizon
