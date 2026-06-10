#pragma once

#include <horizon/Widget.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Button.hpp>
#include <horizon/apt/AptManager.hpp>
#include <functional>

namespace horizon::appstore {

class UpdatesView : public horizon::Widget {
public:
    explicit UpdatesView(std::shared_ptr<horizon::apt::AptManager> apt_manager);
    ~UpdatesView() override;

    std::function<void(bool loading, const std::string& msg)> on_loading_state_changed;
    std::function<void(int update_count)> on_updates_status_changed;

    void load_initial_data();
    void check_for_updates();
    void trigger_update_all();

private:
    std::shared_ptr<horizon::apt::AptManager> m_apt;
    
    horizon::Widget* m_empty_state = nullptr;
    horizon::Widget* m_results_state = nullptr;
    horizon::TableView<horizon::apt::PackageInfo>* m_tableview = nullptr;
    horizon::Button<horizon::AquaObject>* m_btn_check = nullptr;

    std::shared_ptr<bool> m_is_alive;

    void setup_ui();
};

}
