#pragma once

#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>

namespace horizon
{
    class LaunchpadApplication : public Application
    {
    public:
        LaunchpadApplication();
        ~LaunchpadApplication() override;

    private:
        WaylandLayerWindow *m_window{nullptr};
    };
} // namespace horizon
