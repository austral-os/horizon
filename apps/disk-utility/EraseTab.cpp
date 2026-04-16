#include "EraseTab.hpp"
#include "horizon/AirObject.hpp"
#include "horizon/Spacer.hpp"
#include <horizon/Application.hpp>
#include <horizon/SolidObject.hpp>
#include <iostream>

namespace horizon::disks
{
    EraseTab::EraseTab(DiskManager &dm) : m_disk_manager(dm)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);
        set_spacing(20);
        setup_ui();
    }

    void EraseTab::setup_ui()
    {

        add_child(Spacer());

        auto title =
            std::make_unique<Label>("Para borrar toda la información de un disco o volumen:");
        title->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        title->set_fixed_size(20);
        add_child(std::move(title));

        auto instructions = std::make_unique<Label>(
            "1. Seleccione el disco o volumen en la lista de la izquierda.\n"
            "2. Especifique formato y nombre.\n"
            "3. Haga clic en Borrar.");
        instructions->set_fixed_size(100);
        add_child(std::move(instructions));

        // Form fields
        auto form_container = std::make_unique<Widget>();
        form_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        form_container->set_spacing(5);
        form_container->set_fixed_size(30);

        auto format_label = std::make_unique<Label>("Formato:");
        format_label->set_fixed_size(120);

        auto combo = std::make_unique<Combo>();
        combo->add_item("ext4", "Linux Extended (EXT4)");
        combo->add_item("ext3", "Linux Extended (EXT3)");
        combo->add_item("ntfs", "Windows NT (NTFS)");
        combo->add_item("exfat", "Soporte Universal (exFAT)");
        combo->add_item("fat32", "MS-DOS (FAT32)");
        combo->set_selected_item_by_id("ext4");
        m_format_combo = combo.get();

        form_container->add_child(std::move(format_label));
        form_container->add_child(std::move(combo));
        add_child(std::move(form_container));

        auto name_container = std::make_unique<Widget>();
        name_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        name_container->set_spacing(5);
        name_container->set_fixed_size(30);

        auto name_label = std::make_unique<Label>("Nombre:");
        name_label->set_fixed_size(120);

        auto entry = std::make_unique<TextBox<horizon::TextPolicy>>();
        entry->set_placeholder("Sin título");
        entry->set_fixed_size(-1);
        m_name_entry = entry.get();

        name_container->add_child(std::move(name_label));
        name_container->add_child(std::move(entry));
        add_child(std::move(name_container));

        // Action Buttons
        auto button_container = std::make_unique<Widget>();
        button_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_spacing(5);
        button_container->set_fixed_size(30);

        auto erase_btn = std::make_unique<Button<AirObject>>();
        erase_btn->set_text("Borrar...");
        erase_btn->set_fixed_size(150);
        erase_btn->set_accent_color(WidgetAccentColor::Default);
        erase_btn->when_click.connect([this](MouseButtonEventContext &)
                                      { this->on_erase_clicked(); });

        button_container->add_child(std::move(erase_btn));
        button_container->add_child(Spacer());
        add_child(std::move(button_container));

        add_child(Spacer());
    }

    void EraseTab::set_selected_partition(const std::string &path)
    {
        m_selected_path = path;
        // Update labels or state if needed
    }

    void EraseTab::on_erase_clicked()
    {
        if (m_selected_path.empty())
            return;

        std::string name = m_name_entry->text();
        if (name.empty())
            name = "UNTITLED";

        auto selected = m_format_combo->selected_item();
        if (!selected)
            return;

        // Perform the operation (Assuming UI is handled for now)
        auto result = m_disk_manager.format_partition(m_selected_path, selected->id, name);

        // Show notification or dialog (Placeholder)
        std::cout << "Operation: " << result.message << std::endl;
    }

} // namespace horizon::disks
