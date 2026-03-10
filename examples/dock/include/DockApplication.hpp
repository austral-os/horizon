#pragma once

#include <horizon/CompositorAppInterface.hpp>
#include <horizon/LayerApplication.hpp>
#include <memory>
#include <string>
#include <vector>

namespace horizon
{

    class DockShelf;

    struct PinnedApp
    {
        std::string app_id;
        std::string name;
        std::string icon;
        std::string run_id;
    };

    class DockApplication : public LayerApplication
    {
    public:
        DockApplication();
        ~DockApplication() override;

        CompositorAppInterface *compositor_apps() override;

    private:
        void detect_environment();
        void setup_ui();
        void setup_ipc();
        void update_dock(const std::vector<ApplicationInfo> &apps);

        bool _is_wayfire = false;
        DockShelf *_shelf_ptr = nullptr;
        std::unique_ptr<CompositorAppInterface> _compositor_apps;
        static const std::vector<PinnedApp> PINNED_APPS;
    };

} // namespace horizon
