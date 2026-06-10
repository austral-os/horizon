#pragma once

#include <horizon/Widget.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Label.hpp>
#include <horizon/Icon.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/Image.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/apt/AptManager.hpp>
#include <horizon/apt/AppStoreClient.hpp>
#include <functional>
#include <optional>

namespace horizon::appstore {

struct CategoryItem {
    std::string name;
    std::string icon;
};

class PackageDetailsWidget : public horizon::Widget {
public:
    PackageDetailsWidget();
    ~PackageDetailsWidget() override = default;

    void calculate_layout() override;
    void draw(horizon::GraphicsContext& gc) override;

    void update_basic_info(const horizon::apt::PackageInfo& pkg);
    void update_api_info(const horizon::apt::AppDetails& app_details, const horizon::apt::AppVersionDetails& v_details);
    void add_screenshot(const std::string& local_path);
    void clear_screenshots();
    
    horizon::Icon* icon() const { return m_icon; }

private:
    horizon::Icon* m_icon = nullptr;
    horizon::Label* m_title = nullptr;
    horizon::Label* m_version = nullptr;
    horizon::ProgressBar* m_rating = nullptr;
    horizon::Label* m_description = nullptr;
    std::vector<horizon::Image*> m_screenshots;
};

class ExploreView : public horizon::Widget {
public:
    ExploreView(horizon::apt::AptManager* apt_manager);
    ~ExploreView() override = default;

    void perform_search(const std::string& query);
    void load_initial_data();
    void reload_current_view();
    void clear_selection();

    std::function<void(bool, const std::string&)> on_loading_state_changed;
    std::function<void(const horizon::apt::PackageInfo*)> on_package_selected;
    
    const horizon::apt::PackageInfo* selected_package() const { return m_selected_pkg ? &(*m_selected_pkg) : nullptr; }
    
    // Will be called by AppStoreWindow when the Toolbar button is clicked
    void trigger_install();
    void trigger_remove();

private:
    void setup_ui();
    void build_categories();
    void update_details(const horizon::apt::PackageInfo& pkg);
    void filter_by_category(const std::string& category_name);

    horizon::apt::AptManager* m_apt = nullptr;
    horizon::apt::AppStoreClient m_api_client;
    
    horizon::VPanel* m_vpanel = nullptr;
    horizon::TableView<CategoryItem>* m_category_table = nullptr;
    horizon::TableView<horizon::apt::PackageInfo>* m_tableview = nullptr;
    
    PackageDetailsWidget* m_details_widget = nullptr;
    horizon::ScrollArea* m_details_scroll = nullptr;
    horizon::Widget* m_no_sel_widget = nullptr;

    size_t m_search_timer_id = 0;
    std::optional<horizon::apt::PackageInfo> m_selected_pkg;
    std::string m_current_search_query;
    std::string m_current_category;
};

}
