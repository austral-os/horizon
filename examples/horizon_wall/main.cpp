#include <filesystem>
#include <horizon/Image.hpp>
#include <horizon/LayerApplication.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <iostream>
#include <memory>

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        // Create a layer application in the BACKGROUND layer (0)
        auto app = std::make_unique<LayerApplication>("horizon_wall", 0);
        app->set_app_id("horizon_wall");
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
            std::cout << "[HORIZON WALL] Loading wallpaper: " << wall_path << std::endl;
            wallpaper->set_path(wall_path);
            wallpaper->set_mode(ImageMode::Stretch); // Match screen size
        }
        else
        {
            std::cerr << "[HORIZON WALL] Warning: No wallpaper image found." << std::endl;
        }

        // Add dummy click listener to root widget to verify interactivity
        // Note: Image widget might consume clicks if not handled, but root should catch them as
        // fallback or we can add it directly to wallpaper as well if icons will be children of it.

        wallpaper->add_on_mouse_press(
            [](int button)
            {
                std::cout << "[HORIZON WALL] Wallpaper clicked with button: " << button
                          << std::endl;
            });

        root->add_child(std::move(wallpaper));
        app->set_root(std::move(root));

        std::cout << "Horizon Wallpaper started (Background Layer)." << std::endl;

        app->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
