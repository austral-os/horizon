#pragma once

#include <horizon/Widget.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Label.hpp>
#include <horizon/Icon.hpp>
#include <horizon/apt/AptManager.hpp>
#include <horizon/apt/AptManager.hpp>
#include <functional>
#include <optional>

namespace horizon::appstore {

struct CategoryItem {
    std::string name;
    std::string icon;
};

class ExploreView : public horizon::Widget {
public:
    ExploreView(horizon::apt::AptManager* apt_manager);
    ~ExploreView() override = default;

    void perform_search(const std::string& query);
    void load_initial_data();

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
    
    horizon::VPanel* m_vpanel = nullptr;
    horizon::TableView<CategoryItem>* m_category_table = nullptr;
    horizon::TableView<horizon::apt::PackageInfo>* m_tableview = nullptr;
    
    horizon::Icon* m_detail_icon = nullptr;
    horizon::Label* m_detail_title = nullptr;
    horizon::Label* m_detail_desc = nullptr;

    size_t m_search_timer_id = 0;
    std::optional<horizon::apt::PackageInfo> m_selected_pkg;
};

}
