#include <algorithm>
#include <cctype>
#include <cmath>
#include <horizon/Icon.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/LayerApplication.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <vector>

using namespace horizon;

struct PinnedApp
{
    std::string app_id;
    std::string name;
    std::string icon;
    std::string run_id; // ID used for run_app signal
};

const std::vector<PinnedApp> PINNED_APPS = {
    {"arkfm", "Ark File Manager", "arkfm", "arkfm"},
    {"alacritty", "Terminal", "utilities-terminal", "terminal"},
    {"firefox", "Web Browser", "firefox", "firefox"}};

/**
 * @brief Custom widget mimicking the Mac OS X Mountain Lion 3D Dock shelf.
 */
class DockShelf : public Widget
{
public:
    DockShelf()
    {
        // Horizontal layout with padding for the shelf appearance
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);
        // Leave space at the bottom for the 3D lip of the shelf
        set_margin(20);
        set_size(0, 100);
    }

    void calculate_layout() override
    {
        // 1. Calculate the required content width based on children
        int content_width = 0;
        for (const auto &child : children())
        {
            if (child->is_visible() && child->fixed_size() > 0)
            {
                content_width += child->fixed_size() + spacing();
            }
        }
        if (!children().empty() && content_width > 0)
        {
            content_width -= spacing();
        }

        content_width += 40; // Simulated Left + Right padding

        // 2. Update size BEFORE base calculate_layout so parents and centering use new dimensions
        set_size(content_width, height());
        set_fixed_size(content_width);

        Widget::calculate_layout();
    }

    void draw(GraphicsContext &gc) override
    {
        // Clear the widget area to prevent redrawing artifacts over old frames
        gc.clearRect(x(), y(), width(), height());

        // Draw the 3D OS X Mountain Lion Shelf
        // 1. Translucent top surface (Trapezoid)
        // 2. Front edge (Rectangle with rounding and gradient)
        // 3. Glare/Shine

        float w = width();
        float h = height();

        // Shelf geometry measurements
        float lip_height = 10.0f;
        float tray_top_y = 15.0f; // Tray starts 15px down from the top bounds
        float tray_bottom_y = h - lip_height - 2.0f;

        float perspective_offset = 20.0f; // Trapezoid slant width

        // 1. Draw the tray surface (Trapezoid from background to foreground)
        std::vector<PolygonPoint> surface_points = {
            {static_cast<int>(x() + perspective_offset), static_cast<int>(y() + tray_top_y),
             0}, // Top Left
            {static_cast<int>(x() + w - perspective_offset), static_cast<int>(y() + tray_top_y),
             0},                                                                   // Top Right
            {static_cast<int>(x() + w), static_cast<int>(y() + tray_bottom_y), 0}, // Bottom Right
            {static_cast<int>(x()), static_cast<int>(y() + tray_bottom_y), 0}      // Bottom Left
        };

        // Soft white translucent gradient for the glass shelf
        gc.fillLinearGradientPolygon(surface_points, Color(1.0f, 1.0f, 1.0f, 0.2f),
                                     Color(1.0f, 1.0f, 1.0f, 0.6f),
                                     true // vertical
        );

        // 2. Draw the front lip (Edge)
        gc.fillLinearGradientRect(x(), y() + tray_bottom_y, w, lip_height,
                                  Color(0.8f, 0.8f, 0.8f, 0.9f), Color(0.4f, 0.4f, 0.4f, 0.8f),
                                  true, // vertical
                                  CornerRadius(0, 0, 4, 4));

        // 3. Glare/Shine effects
        // Top edge of the lip shine
        gc.setColor(Color(1.0f, 1.0f, 1.0f, 0.8f));
        gc.drawLine(x(), y() + tray_bottom_y + 1, x() + w, y() + tray_bottom_y + 1, 1.0f);

        // Bottom edge of the lip shadow
        gc.setColor(Color(0.1f, 0.1f, 0.1f, 0.5f));
        gc.drawLine(x() + 4, y() + tray_bottom_y + lip_height, x() + w - 4,
                    y() + tray_bottom_y + lip_height, 1.0f);

        // Separating line acting as the back wall intersection
        gc.setColor(Color(1.0f, 1.0f, 1.0f, 0.3f));
        gc.drawLine(x() + perspective_offset, y() + tray_top_y, x() + w - perspective_offset,
                    y() + tray_top_y, 1.0f);

        // Reset color or state if necessary (none required physically for Horizon context without
        // states)
        gc.setColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }
};

int main(int argc, char *argv[])
{
    try
    {
        // 1. Create the overlay application
        // Anchor to the bottom, left, and right edge to allow size 0
        auto app = std::make_unique<LayerApplication>("dock", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY);
        app->set_name("Dock");

        app->set_anchor(2 | 4 | 8); // BOTTOM | LEFT | RIGHT
        app->set_size(0, 100);
        app->set_exclusive_zone(100);
        app->set_show_in_dock(false);
        app->set_show_in_system_tray(false);
        app->set_visible(true);             // Enable input region
        app->set_keyboard_interactivity(0); // NONE - prevent focus stealing in labwc

        const char *desktop_env = getenv("XDG_CURRENT_DESKTOP");
        std::string desktop_str = desktop_env ? desktop_env : "";
        std::transform(desktop_str.begin(), desktop_str.end(), desktop_str.begin(), ::tolower);
        bool is_wayfire = (desktop_str.find("wayfire") != std::string::npos);
        if (is_wayfire)
        {
            LOG_INFO << "[DOCK] Wayfire detected, using foreign-toplevel for restoration.";
        }

        // 2. Root Window
        auto root = std::make_unique<Widget>();
        // Center the shelf horizontally within the available width
        // By setting cross axis alignment to center
        root->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        // Make background completely transparent
        root->set_background_color({0.0f, 0.0f, 0.0f, 0.0f});

        // 3. The Dock Shelf
        auto shelf = std::make_unique<DockShelf>();

        // Capture raw pointer to shelf before it's moved
        DockShelf *shelf_ptr = shelf.get();

        // Add shelf to root. We use a spacer approach to center the shelf properly
        // because standard layout alignment doesn't strictly center horizontally in horizontal
        // layout yet
        auto left_spacer = std::make_unique<Widget>();
        auto right_spacer = std::make_unique<Widget>();

        root->add_child(std::move(left_spacer));
        root->add_child(std::move(shelf));
        root->add_child(std::move(right_spacer));

        app->set_root(std::move(root));

        // 5. Dock Update Logic
        auto update_dock = [app_ptr = app.get(), shelf_ptr, is_wayfire](const nlohmann::json &apps)
        {
            LOG_INFO << "Updating Dock icons...";
            shelf_ptr->clear_children();

            // Track which pinned apps are already running
            std::set<std::string> running_pinned_ids;

            // 1. Add Pinned Apps (checking if they are running)
            for (const auto &pinned : PINNED_APPS)
            {
                bool is_running = false;
                nlohmann::json running_app_data;

                for (const auto &app_j : apps)
                {
                    std::string app_id = app_j.value("app_id", "");
                    // Simple match by app_id
                    if (app_id.find(pinned.app_id) != std::string::npos)
                    {
                        is_running = true;
                        running_app_data = app_j;
                        running_pinned_ids.insert(app_id);
                        break;
                    }
                }

                auto icn = std::make_unique<Icon>();
                icn->set_icon_name(pinned.icon);
                icn->set_icon_size(48);
                icn->set_margin(5);

                if (is_running)
                {
                    int pid = running_app_data.value("pid", -1);
                    std::string app_id = running_app_data.value("app_id", "");
                    bool is_minimized = running_app_data.value("is_minimized", false);

                    icn->when_mouse_press.connect(
                        [app_ptr, pid, app_id, is_wayfire,
                         is_minimized](MouseButtonEventContext &ctx)
                        {
                            if (pid == -1)
                                return;

                            auto send_sig =
                                [pid](const std::string &sig_name, const std::string &token = "")
                            {
                                try
                                {
                                    nlohmann::json sig;
                                    sig["type"] = "send_signal";
                                    sig["target_pid"] = pid;
                                    sig["signal"] = sig_name;
                                    if (!token.empty())
                                        sig["token"] = token;

                                    IpcClient client("/tmp/horizon_session.sock");
                                    client.send(sig.dump());
                                }
                                catch (...)
                                {
                                }
                            };

                            if (ctx.button == 274) // BTN_MIDDLE
                            {
                                send_sig("close");
                                return;
                            }

                            if (is_minimized)
                            {
                                if (is_wayfire && !app_id.empty())
                                {
                                    app_ptr->w_surface()->restore_foreign_app(app_id);
                                }
                                else
                                {
                                    app_ptr->w_surface()->request_activation_token(
                                        [send_sig](const std::string &token)
                                        { send_sig("restore", token); }, ctx.serial);
                                }
                            }
                            else
                            {
                                send_sig("minimize");
                            }
                        });
                }
                else
                {
                    // Not running, clicking launches it via horizon_session signal
                    icn->when_mouse_press.connect(
                        [app_ptr, run_id = pinned.run_id](MouseButtonEventContext &ctx)
                        {
                            if (ctx.button == 272) // BTN_LEFT
                            {
                                LOG_INFO << "[DOCK] Requesting to run app: " << run_id;
                                app_ptr->send_remote_signal(-1, "run_app", run_id);
                            }
                        });
                }

                shelf_ptr->add_child(std::move(icn));
            }

            // 2. Add Separator (if there are other running apps)
            bool has_other_apps = false;
            for (const auto &app_j : apps)
            {
                if (app_j.value("show_in_dock", false))
                {
                    std::string app_id = app_j.value("app_id", "");
                    if (running_pinned_ids.find(app_id) == running_pinned_ids.end())
                    {
                        has_other_apps = true;
                        break;
                    }
                }
            }

            if (has_other_apps)
            {
                auto separator = std::make_unique<Widget>();
                separator->set_fixed_size(2);
                separator->set_margin(10);
                separator->set_background_color({1.0f, 1.0f, 1.0f, 0.3f});
                shelf_ptr->add_child(std::move(separator));
            }

            // 3. Add Other Running Apps
            for (const auto &app_j : apps)
            {
                if (app_j.value("show_in_dock", false))
                {
                    std::string app_id = app_j.value("app_id", "");
                    if (running_pinned_ids.find(app_id) != running_pinned_ids.end())
                    {
                        continue; // Already added as pinned
                    }

                    auto icn = std::make_unique<Icon>();
                    icn->set_icon_name(app_j.value("icon", "application-x-executable"));
                    icn->set_icon_size(48);
                    icn->set_margin(5);

                    int pid = app_j.value("pid", -1);
                    std::string app_id_str = app_j.value("app_id", "");
                    bool is_minimized = app_j.value("is_minimized", false);

                    icn->when_mouse_press.connect(
                        [app_ptr, pid, app_id_str, is_wayfire,
                         is_minimized](MouseButtonEventContext &ctx)
                        {
                            if (pid == -1)
                                return;

                            auto send_sig =
                                [pid](const std::string &sig_name, const std::string &token = "")
                            {
                                try
                                {
                                    nlohmann::json sig;
                                    sig["type"] = "send_signal";
                                    sig["target_pid"] = pid;
                                    sig["signal"] = sig_name;
                                    if (!token.empty())
                                        sig["token"] = token;

                                    IpcClient client("/tmp/horizon_session.sock");
                                    client.send(sig.dump());
                                }
                                catch (...)
                                {
                                }
                            };

                            if (ctx.button == 274) // BTN_MIDDLE
                            {
                                send_sig("close");
                                return;
                            }

                            if (is_minimized)
                            {
                                if (is_wayfire && !app_id_str.empty())
                                {
                                    app_ptr->w_surface()->restore_foreign_app(app_id_str);
                                }
                                else
                                {
                                    app_ptr->w_surface()->request_activation_token(
                                        [send_sig](const std::string &token)
                                        { send_sig("restore", token); }, ctx.serial);
                                }
                            }
                            else
                            {
                                send_sig("minimize");
                            }
                        });

                    shelf_ptr->add_child(std::move(icn));
                }
            }
            shelf_ptr->calculate_layout();
            app_ptr->invalidate(); // Trigger full repaint for transparent surface
        };

        // Initial update with empty apps list to show pinned apps
        app->post_task([update_dock]() { update_dock(nlohmann::json::array()); });

        // 6. Subscribe to Horizon Session Broker
        IpcClient client("/tmp/horizon_session.sock");
        client.subscribe("{\"type\": \"subscribe\"}",
                         [app_ptr = app.get(), update_dock](const std::string &msg)
                         {
                             try
                             {
                                 auto j = nlohmann::json::parse(msg);
                                 if (j.value("type", "") == "app_list_updated")
                                 {
                                     auto apps = j.at("apps");
                                     // Update UI on the main thread
                                     app_ptr->post_task([update_dock, apps]()
                                                        { update_dock(apps); });
                                 }
                             }
                             catch (const std::exception &e)
                             {
                                 LOG_ERROR << "Dock: Error parsing broadcast: " << e.what();
                             }
                         });

        LOG_INFO << "Starting Mountain Lion OS X Dock Overlay...";
        app->run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }
    return 0;
}
