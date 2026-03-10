#pragma once

#include <horizon/IpcClient.hpp>
#include <horizon/LayerApplication.hpp>
#include <memory>
#include <nlohmann/json.hpp>
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

    private:
        void detect_environment();
        void setup_ui();
        void setup_ipc();
        void update_dock(const nlohmann::json &apps);

        bool _is_wayfire = false;
        DockShelf *_shelf_ptr = nullptr;
        std::unique_ptr<IpcClient> _ipc_client;
        static const std::vector<PinnedApp> PINNED_APPS;
    };

} // namespace horizon
