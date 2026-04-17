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
    class DiskUtilityWindow : public ApplicationWindow
    {
    public:
        DiskUtilityWindow();
        ~DiskUtilityWindow() = default;

    private:
        void setup_toolbar();
        void setup_layout();
        void populate_devices();
        void on_item_selected(TreeViewItem* item);
        void check_for_hardware_changes();
        
        DiskManager m_disk_manager;
        std::unique_ptr<dbusutils::DbusHelper> m_dbus_helper;
        TreeView* m_device_tree{nullptr};
        Notebook* m_notebook{nullptr};
        EraseTab* m_erase_tab{nullptr};
        DiskInfoWidget* m_info_panel{nullptr};
    };
} // namespace horizon::disks
