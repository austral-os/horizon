#pragma once

#include <horizon/CompositorAppInterface.hpp>
#include <horizon/Icon.hpp>
#include <horizon/LayerApplication.hpp>
#include <functional>
#include <string>

namespace horizon
{

    class DockItem : public Icon
    {
    public:
        DockItem(LayerApplication *app, const std::string &icon_name, bool is_wayfire);

        void set_app_info(const ApplicationInfo &info);
        void set_pinned_data(const std::string &run_id);

        // Called when the user right-clicks this item. Receives absolute screen (x, y).
        std::function<void(int, int)> on_right_click;

    private:
        void send_sig(const std::string &sig_name, const std::string &token = "");
        void setup_running_behavior();
        void setup_pinned_behavior();

        LayerApplication *_app;
        bool _is_wayfire;
        int _pid = -1;
        std::string _app_id;
        std::string _run_id;
        bool _is_minimized = false;
        bool _is_running = false;
    };

} // namespace horizon
