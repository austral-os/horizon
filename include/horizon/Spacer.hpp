#pragma once

#include "horizon/Widget.hpp"
#include <memory>

namespace horizon
{
    std::unique_ptr<Widget> Spacer(int fs);
    std::unique_ptr<Widget> Spacer(void);
} // namespace horizon