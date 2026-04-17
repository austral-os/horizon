#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Combo.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon-disk-utilities/DiskManager.hpp>

namespace horizon::disks
{
    class EraseTab : public Widget
    {
    public:
        EraseTab(DiskManager& dm);
        ~EraseTab() = default;

        void update_selection(DiskDevice* disk, DiskPartition* partition);

    private:
        void setup_ui();
        void on_erase_clicked();

        DiskManager& m_disk_manager;
        DiskDevice* m_selected_disk{nullptr};
        DiskPartition* m_selected_partition{nullptr};
        
        Label* m_status_label{nullptr};
        Combo* m_format_combo{nullptr};
        TextBox<horizon::TextPolicy>* m_name_entry{nullptr};
    };
} // namespace horizon::disks
