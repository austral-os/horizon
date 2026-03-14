#include <filesystem>
#include <horizon/Image.hpp>
#include <horizon/Logger.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <memory>

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        // Create a layer application in the BACKGROUND layer (0)
        auto app = std::make_unique<WaylandLayerWindow>("horizon_wall", 0);
        app->set_name("Horizon Wallpaper");
        app->set_icon_name("preferences-desktop-wallpaper");

        // Anchor to ALL sides to cover the entire screen
        app->set_anchor(ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

        // Set exclusive zone to -1 so it stays behind panels and other windows
        app->set_exclusive_zone(-1);

        app->set_show_in_dock(false);
        app->set_visible(true);

        // Root widget
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        // Wallpaper Image
        auto wallpaper = std::make_unique<Image>();

        // Find a default wallpaper or use the one provided via CLI
        std::string wall_path = (argc > 1) ? argv[1] : "";
        if (wall_path.empty())
        {
            // Tentative default paths
            std::vector<std::string> search_paths = {
                "examples/config/bg.png",
                "/home/horacio/Desarrollo/austral-os/horizon/examples/config/bg.png"};

            for (const auto &p : search_paths)
            {
                if (std::filesystem::exists(p))
                {
                    wall_path = p;
                    break;
                }
            }
        }

        if (!wall_path.empty())
        {
            LOG_INFO << "[HORIZON WALL] Loading wallpaper: " << wall_path;
            wallpaper->set_path(wall_path);
            wallpaper->set_mode(ImageMode::Stretch); // Match screen size
        }
        else
        {
            LOG_ERROR << "[HORIZON WALL] Warning: No wallpaper image found.";
        }

        // Add dummy click listener to root widget to verify interactivity
        // Note: Image widget might consume clicks if not handled, but root should catch them as
        // fallback or we can add it directly to wallpaper as well if icons will be children of it.

        wallpaper->add_on_mouse_press(
            [](int button)
            { LOG_INFO << "[HORIZON WALL] Wallpaper clicked with button: " << button; });

        root->add_child(std::move(wallpaper));
        app->set_root(std::move(root));

        LOG_INFO << "Horizon Wallpaper started (Background Layer).";

        app->run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }

    return 0;
}
