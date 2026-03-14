#include "WallApplication.hpp"
#include "horizon/EventsManager.hpp"
#include <filesystem>
#include <horizon/Image.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <vector>

namespace horizon
{
    WallApplication::WallApplication(const std::string &wall_path)
        : Application("org.horizon.wall", 1920, 1080, true, true)
    {
        m_window = create_layer_window("horizon_wall", 0); // Background layer

        m_window->set_name("Horizon Wallpaper");
        m_window->set_icon_name("preferences-desktop-wallpaper");
        m_window->set_show_in_dock(false);

        setup_window();
        load_wallpaper(wall_path);

        m_window->set_visible(true);
        LOG_INFO << "Horizon Wallpaper initialized.";
    }

    WallApplication::~WallApplication() = default;

    void WallApplication::setup_window()
    {
        m_window->set_anchor(
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
            ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
        m_window->set_exclusive_zone(-1);
    }

    void WallApplication::load_wallpaper(const std::string &wall_path)
    {
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto wallpaper = std::make_unique<Image>();

        std::string final_path = wall_path;
        if (final_path.empty())
        {
            std::vector<std::string> search_paths = {
                "examples/config/bg.png",
                "/home/horacio/Desarrollo/austral-os/horizon/examples/config/bg.png"};

            for (const auto &p : search_paths)
            {
                if (std::filesystem::exists(p))
                {
                    final_path = p;
                    break;
                }
            }
        }

        if (!final_path.empty())
        {
            LOG_INFO << "[HORIZON WALL] Loading wallpaper: " << final_path;
            wallpaper->set_path(final_path);
            wallpaper->set_mode(ImageMode::Stretch);
        }
        else
        {
            LOG_ERROR << "[HORIZON WALL] Warning: No wallpaper image found.";
        }

        wallpaper->when_mouse_press.connect(
            [](MouseButtonEventContext &ev)
            { LOG_INFO << "[HORIZON WALL] Wallpaper clicked with button: " << ev.button; });

        root->add_child(std::move(wallpaper));
        m_window->set_root(std::move(root));
    }
} // namespace horizon
