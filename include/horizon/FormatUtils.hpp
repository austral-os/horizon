#pragma once

#include <string>

namespace horizon
{
    /**
     * @brief A helper formatter for bytes (B, KB, MB, GB, TB, PB).
     */
    std::string format_bytes(double bytes);
} // namespace horizon
