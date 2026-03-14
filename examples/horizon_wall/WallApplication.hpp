#pragma once

#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <string>

namespace horizon
{
    class WallApplication : public Application
    {
    public:
        WallApplication(const std::string &wall_path = "");
        ~WallApplication() override;

    private:
        void setup_window();
        void load_wallpaper(const std::string &wall_path);

        WaylandLayerWindow *m_window{nullptr};
    };
} // namespace horizon
