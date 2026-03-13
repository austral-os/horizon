#pragma once

#include "AppItem.hpp"
#include <horizon/IconView.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Window.hpp>
#include <horizon/Widget.hpp>
#include <string>
#include <vector>

namespace horizon
{

    class LaunchpadWindow : public Window
    {
    public:
        LaunchpadWindow(Application* app);

        void draw(GraphicsContext &gc) override;

    private:
        void load_apps();
        void filter_apps(const std::string &query);

        SearchBox *m_search_box{nullptr};
        IconView<AppData> *m_icon_view{nullptr};

        std::vector<AppData> m_all_apps;
        std::vector<AppData> m_filtered_apps;
    };

} // namespace horizon
