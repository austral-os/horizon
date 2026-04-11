#pragma once

#include "horizon/Sidebar.hpp"

namespace horizon::files
{
    class FileSidebar : public Sidebar
    {
    public:
        FileSidebar();
        ~FileSidebar() override = default;
    };
} // namespace horizon::files
