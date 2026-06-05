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
        DockItem(WaylandLayerWindow *app, CompositorAppInterface *compositor_apps, const std::string &icon_name, bool is_meteor);

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
        void set_app_id(const std::string &app_id)
        {
            _app_id = app_id;
        }
        const std::string &run_id() const
        {
            return _run_id;
        }
        const std::string &name() const
        {
            return _name;
        }
        void set_name(const std::string &name)
        {
            _name = name;
        }
        bool is_running() const
        {
            return !_instances.empty();
        }

        bool is_pinned() const
        {
            return !_run_id.empty();
        }

        void set_dragging(bool dragging) { _dragging = dragging; invalidate(); }
        bool is_dragging() const { return _dragging; }

        void draw(GraphicsContext &ctx) override;

    private:
        void send_sig(const std::string &sig_name, const std::string &token = "");
        void setup_running_behavior();
        void setup_pinned_behavior();
        void setup_drag_behavior();

        WaylandLayerWindow *_app;
        CompositorAppInterface *_compositor_apps;
        bool _is_meteor;
        std::string _app_id;
        std::string _run_id;
        std::string _name;
        std::vector<ApplicationInfo> _instances;
        bool _dragging = false;
        double _press_x = 0;
        double _press_y = 0;
        int _last_rect_x = -1;
        int _last_rect_y = -1;
        int _last_rect_w = -1;
        int _last_rect_h = -1;
    };

} // namespace horizon
