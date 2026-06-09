#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/ToggleGroupButton.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Label.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/apt/AptManager.hpp>
#include "views/FeaturedView.hpp"
#include "views/ExploreView.hpp"
#include "views/UpdatesView.hpp"

namespace horizon::appstore {

class AppStoreWindow : public horizon::ApplicationWindow {
public:
    AppStoreWindow();
    ~AppStoreWindow() override = default;

private:
    void setup_toolbar();
    void setup_statusbar();
    void setup_content();
    
    void set_status(const std::string& message, bool loading = false);

    horizon::ToggleGroupButton* m_group_btn = nullptr;
    horizon::ToolbarButton* m_btn_action = nullptr;
    horizon::SearchBox* m_search_box = nullptr;
    horizon::Widget* m_content_area = nullptr;
    horizon::Label* m_status_label = nullptr;
    horizon::ProgressBar* m_progress_bar = nullptr;
    
    std::unique_ptr<horizon::apt::AptManager> m_apt_manager;
    FeaturedView* m_featured_view = nullptr;
    ExploreView* m_explore_view = nullptr;
    UpdatesView* m_updates_view = nullptr;
};

}
