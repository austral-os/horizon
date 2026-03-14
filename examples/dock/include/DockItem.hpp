#pragma once

#include <functional>
#include <horizon/CompositorAppInterface.hpp>
#include <horizon/Icon.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <string>

namespace horizon
{

    class DockItem : public Icon
    {
    public:
        DockItem(WaylandLayerWindow *app, const std::string &icon_name, bool is_wayfire);

        void set_app_info(const ApplicationInfo &info);
        void set_pinned_data(const std::string &run_id);
        void set_run_id(const std::string &run_id)
        {
            _run_id = run_id;
        }

        // Called when the user right-clicks this item. Receives absolute screen (x, y).
        std::function<void(int, int)> on_right_click;

        int pid() const
        {
            return _pid;
        }
        uintptr_t instance_id() const
        {
            return _instance_id;
        }
        const std::string &app_id() const
        {
            return _app_id;
        }
        const std::string &run_id() const
        {
            return _run_id;
        }
        bool is_running() const
        {
            return _is_running;
        }

    private:
        void send_sig(const std::string &sig_name, const std::string &token = "");
        void setup_running_behavior();
        void setup_pinned_behavior();

        WaylandLayerWindow *_app;
        bool _is_wayfire;
        int _pid = -1;
        std::string _app_id;
        std::string _run_id;
        bool _is_minimized = false;
        bool _is_running = false;
        uintptr_t _instance_id = 0;
    };

} // namespace horizon
