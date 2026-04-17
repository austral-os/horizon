#include "EraseTab.hpp"
#include "horizon/AirObject.hpp"
#include "horizon/Spacer.hpp"
#include <horizon/Application.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Window.hpp>
#include "DiskProgressDialog.hpp"
#include <thread>
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

        auto status = std::make_unique<Label>("Seleccione un ítem de la izquierda.");
        status->set_text_color({0.7f, 0.7f, 0.7f, 1.0f});
        status->set_fixed_size(20);
        m_status_label = status.get();
        add_child(std::move(status));

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

    void EraseTab::update_selection(DiskDevice* disk, DiskPartition* partition)
    {
        m_selected_disk = disk;
        m_selected_partition = partition;

        if (m_selected_partition) {
            m_status_label->set_text("Seleccionado: Partición " + m_selected_partition->device_path + " (" + m_selected_partition->human_capacity() + ")");
            m_name_entry->set_text(m_selected_partition->label);
        } else if (m_selected_disk) {
            m_status_label->set_text("Seleccionado: Disco " + m_selected_disk->device_path + " (" + m_selected_disk->human_capacity() + ")");
            m_name_entry->set_text("");
        } else {
            m_status_label->set_text("Seleccione un ítem de la izquierda.");
        }
    }

    void EraseTab::on_erase_clicked()
    {
        if (!m_selected_disk && !m_selected_partition) {
            application()->alert("Por favor, seleccione un disco o partición para borrar.", "Borrar");
            return;
        }

        auto selected_format = m_format_combo->selected_item();
        if (!selected_format) {
            application()->alert("Seleccione un formato válido.", "Borrar");
            return;
        }

        std::string target_name = m_selected_partition ? m_selected_partition->device_path : m_selected_disk->device_path;
        bool is_internal = m_selected_disk ? !m_selected_disk->is_removable : true;

        std::string confirm_msg = "¿Está seguro de que desea borrar " + target_name + "? Todos los datos se perderán permanentemente.";
        if (is_internal) {
            confirm_msg = "¡ADVERTENCIA! Este es un DISCO INTERNO. " + confirm_msg + "\n\nSe requerirán permisos de administrador.";
        }

        if (!application()->confirm(confirm_msg, "Confirmar Borrado")) {
            return;
        }

        std::string name = m_name_entry->text();
        if (name.empty()) name = "SIN_TITULO";
        std::string fs_type = selected_format->id;
        
        // Capture specific identifying info by value to avoid dangling pointers during refresh
        std::string device_path = m_selected_partition ? m_selected_partition->device_path : m_selected_disk->device_path;
        bool is_partition = (m_selected_partition != nullptr);

        // --- Start Async Erase ---
        
        // 1. Create the dialog
        auto progress_dialog = std::make_shared<DiskProgressDialog>("Borrando...", "Preparando operación...");
        
        // 2. Launch the dialog event loop in its own thread
        std::thread([dialog = progress_dialog]() {
            dialog->initialize();
            dialog->run();
        }).detach();

        // 3. Launch the worker thread for the disk operation
        std::thread([this, name, fs_type, device_path, is_partition, dialog = progress_dialog]() {
            OperationResult result;
            
            auto update_status = [&](const std::string& status) {
                dialog->post_task([dialog, status]() {
                    dialog->set_status(status);
                });
            };

            // Give the dialog a moment to initialize
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            try {
                if (is_partition) {
                    update_status("Borrando partición anterior...");
                    result = this->m_disk_manager.recreate_and_format_partition(
                        device_path, fs_type, name);
                } else {
                    update_status("Preparando tabla GPT...");
                    result = this->m_disk_manager.erase_disk(
                        device_path, fs_type, name);
                }
            } catch (const std::exception& e) {
                result = {false, e.what()};
            }

            // Return to main thread via the dialog's task queue (which we are closing anyway)
            // or directly update UI via application().
            // IMPORTANT: Window::quit() can be called from any thread if it's thread-safe, 
            // but usually we post it to the window's own thread.
            dialog->post_task([this, result, dialog]() mutable {
                dialog->quit(); // This will exit the dialog's run() loop
                
                if (result.success) {
                    this->application()->alert("La operación de borrado se completó con éxito.", "Borrar");
                    
                    // Trigger refresh in main window
                    Widget* p = this->parent();
                    while (p && !dynamic_cast<horizon::Window*>(p)) {
                        p = p->parent();
                    }
                    if (p) {
                        static_cast<horizon::Window*>(p)->signals.emit("disk.refresh");
                    }
                } else {
                    LOG_ERROR << "[DiskUtility] Erase failed: " << result.message;
                    this->application()->alert("Error al borrar: " + result.message, "Borrar", MessageType::Error);
                }
            });
        }).detach();
    }

} // namespace horizon::disks
