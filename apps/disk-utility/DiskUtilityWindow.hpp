#pragma once
#include <horizon/ApplicationWindow.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/TreeView.hpp>
#include <horizon/Notebook.hpp>
#include "EraseTab.hpp"
#include "DiskInfoWidget.hpp"
#include <horizon-disk-utilities/DiskManager.hpp>

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
        
        DiskManager m_disk_manager;
        TreeView* m_device_tree{nullptr};
        Notebook* m_notebook{nullptr};
        EraseTab* m_erase_tab{nullptr};
        DiskInfoWidget* m_info_panel{nullptr};
    };
} // namespace horizon::disks
