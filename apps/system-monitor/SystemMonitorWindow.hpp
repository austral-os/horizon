#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/TableView.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/Spacer.hpp>
#include "ProcessManager.hpp"
#include "CPUStats.hpp"
#include "MemoryStats.hpp"
#include "EnergyStats.hpp"
#include "DiskStats.hpp"
#include "NetworkStats.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace horizon
{
    class SystemMonitorWindow : public ApplicationWindow
    {
    public:
        SystemMonitorWindow();
        ~SystemMonitorWindow() override;

    private:
        struct RefreshSnapshot
        {
            uint64_t generation{0};
            std::vector<ProcessInfo> processes;
            CPUUsage cpu_usage;
            MemoryUsage mem_usage;
            EnergyUsage energy_usage;
            DiskUsage disk_usage;
            NetworkUsage net_usage;
        };

        void setup_toolbar();
        void setup_content();
        void request_process_update();
        void apply_refresh_snapshot(RefreshSnapshot snapshot);

        void set_application_recursive(WaylandWindow* app) override;

        size_t m_refresh_timer_id{0};
        std::atomic<bool> m_refresh_in_progress{false};
        std::atomic<bool> m_refresh_pending{false};
        std::atomic<uint64_t> m_refresh_generation{0};
        std::shared_ptr<std::atomic<bool>> m_alive{std::make_shared<std::atomic<bool>>(true)};
        std::thread m_refresh_thread;
        int m_selected_pid{-1};
        ProcessManager m_process_manager;
        TableView<ProcessInfo>* m_table_view{nullptr};
        ToolbarButton* m_btn_terminate{nullptr};
        SearchBox* m_search_box{nullptr};
        Widget* m_graphs_area{nullptr};
        CPUStats* m_cpu_stats{nullptr};
        MemoryStats* m_memory_stats{nullptr};
        EnergyStats* m_energy_stats{nullptr};
        DiskStats* m_disk_stats{nullptr};
        NetworkStats* m_network_stats{nullptr};
    };
} // namespace horizon
