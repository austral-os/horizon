#pragma once

#include <horizon/Widget.hpp>
#include <horizon/TableView.hpp>
#include <horizon/Button.hpp>
#include <horizon/apt/AptManager.hpp>
#include <functional>

namespace horizon::appstore {

class UpdatesView : public horizon::Widget {
public:
    UpdatesView(horizon::apt::AptManager* apt_manager);
    ~UpdatesView() override = default;

    std::function<void(bool loading, const std::string& msg)> on_loading_state_changed;
    std::function<void(int update_count)> on_updates_status_changed;

    void load_initial_data();
    void check_for_updates();
    void trigger_update_all();

private:
    horizon::apt::AptManager* m_apt = nullptr;
    
    horizon::Widget* m_empty_state = nullptr;
    horizon::Widget* m_results_state = nullptr;
    horizon::TableView<horizon::apt::PackageInfo>* m_tableview = nullptr;
    horizon::Button<horizon::AquaObject>* m_btn_check = nullptr;

    void setup_ui();
};

}
