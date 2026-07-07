#pragma once

#include "DockApplets.hpp"
#include <nlohmann/json.hpp>

namespace horizon
{

    /**
     * @brief Downloads folder applet with parabola fan popup
     */
    class DownloadsApplet : public DockApplet
    {
    public:
        DownloadsApplet(DockApplication *app);
        ~DownloadsApplet() override;

        void load_config(const nlohmann::json &config) override;

    private:
        int m_max_items{9};
    };

} // namespace horizon
