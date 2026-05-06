#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/TableView.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/Spacer.hpp>
#include "ProcessManager.hpp"
#include <memory>
#include <vector>

namespace horizon
{
    class SystemMonitorWindow : public ApplicationWindow
    {
    public:
        SystemMonitorWindow();
        ~SystemMonitorWindow() = default;

    private:
        void setup_toolbar();
        void setup_content();
        void update_processes();

        void set_application_recursive(WaylandWindow* app) override;

        size_t m_refresh_timer_id{0};
        int m_selected_pid{-1};
        ProcessManager m_process_manager;
        TableView<ProcessInfo>* m_table_view{nullptr};
        ToolbarButton* m_btn_terminate{nullptr};
        SearchBox* m_search_box{nullptr};
        Widget* m_graphs_area{nullptr};
    };
} // namespace horizon
