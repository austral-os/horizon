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

        void set_selected_partition(const std::string& path);

    private:
        void setup_ui();
        void on_erase_clicked();

        DiskManager& m_disk_manager;
        std::string m_selected_path;
        
        Label* m_target_label{nullptr};
        Combo* m_format_combo{nullptr};
        TextBox<horizon::TextPolicy>* m_name_entry{nullptr};
    };
} // namespace horizon::disks
