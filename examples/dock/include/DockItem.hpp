#pragma once

#include <horizon/CompositorAppInterface.hpp>
#include <horizon/Icon.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <string>
#include <vector>

namespace horizon
{

    class DockItem : public Icon
    {
    public:
        DockItem(WaylandLayerWindow *app, const std::string &icon_name, bool is_wayfire);

        void add_instance(const ApplicationInfo &info);
        void set_pinned_data(const std::string &run_id);
        void set_run_id(const std::string &run_id)
        {
            _run_id = run_id;
        }

        const std::vector<ApplicationInfo>& instances() const
        {
            return _instances;
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
            return !_instances.empty();
        }

        void draw(GraphicsContext &ctx) override;

    private:
        void send_sig(const std::string &sig_name, const std::string &token = "");
        void setup_running_behavior();
        void setup_pinned_behavior();

        WaylandLayerWindow *_app;
        bool _is_wayfire;
        std::string _app_id;
        std::string _run_id;
        std::vector<ApplicationInfo> _instances;
    };

} // namespace horizon
