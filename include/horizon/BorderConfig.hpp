#pragma once

namespace horizon
{
    struct BorderConfig
    {
        bool left;
        bool right;
        bool top;
        bool bottom;

        BorderConfig() : left(true), right(true), top(true), bottom(true) {}
        BorderConfig(bool left, bool right, bool top, bool bottom)
            : left(left), right(right), top(top), bottom(bottom)
        {
        }
    };
} // namespace horizon