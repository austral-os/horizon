#pragma once
#include <horizon/ApplicationWindow.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/TreeView.hpp>
#include <horizon/Notebook.hpp>
#include "EraseTab.hpp"
#include "DiskInfoWidget.hpp"
#include <horizon-disk-utilities/DiskManager.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>

namespace horizon::disks
{
    struct DiskItemSelectedContext : public EventContext
    {
        DiskDevice* disk{nullptr};
        DiskPartition* partition{nullptr};
    };

    class DiskUtilityWindow : public ApplicationWindow
    {
    public:
        DiskUtilityWindow();
        ~DiskUtilityWindow() = default;

        EventsManager<DiskItemSelectedContext> when_item_selected;
        void initialize_monitoring();

    private:
        void setup_toolbar();
        void setup_layout();
        void populate_devices();
        void on_item_selected(TreeViewItem* item);
        void check_for_hardware_changes();
        void on_mount_requested();
        void on_eject_requested();
        void on_info_requested();
        void update_toolbar_state();
        void refresh_devices();
        
        DiskManager m_disk_manager;
        std::unique_ptr<dbusutils::DbusHelper> m_dbus_helper;
        
        DiskDevice* m_selected_disk{nullptr};
        DiskPartition* m_selected_partition{nullptr};

        TreeView* m_device_tree{nullptr};
        Notebook* m_notebook{nullptr};
        EraseTab* m_erase_tab{nullptr};
        DiskInfoWidget* m_info_panel{nullptr};
        ToolbarButton* m_mount_btn{nullptr};
        ToolbarButton* m_eject_btn{nullptr};
        ToolbarButton* m_info_btn{nullptr};

        std::unique_ptr<dbusutils::DbusHelper> m_monitor_helper;
    };
} // namespace horizon::disks
