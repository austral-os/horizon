#include "EraseTab.hpp"
#include <horizon/I18n.hpp>
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
            std::make_unique<Label>(horizon::i18n().tr("disk_utility.erase.title"));
        title->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        title->set_fixed_size(20);
        add_child(std::move(title));

        auto status = std::make_unique<Label>(horizon::i18n().tr("disk_utility.erase.selection_hint"));
        status->set_text_color({0.7f, 0.7f, 0.7f, 1.0f});
        status->set_fixed_size(20);
        m_status_label = status.get();
        add_child(std::move(status));

        auto instructions = std::make_unique<Label>(horizon::i18n().tr("disk_utility.erase.instructions"));
        instructions->set_fixed_size(100);
        add_child(std::move(instructions));

        // Form fields
        auto form_container = std::make_unique<Widget>();
        form_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        form_container->set_spacing(5);
        form_container->set_fixed_size(30);

        auto format_label = std::make_unique<Label>(horizon::i18n().tr("disk_utility.erase.label_format"));
        format_label->set_fixed_size(120);

        auto combo = std::make_unique<Combo>();
        
        auto supported = m_disk_manager.get_supported_filesystems();
        bool first = true;
        for (const auto& fmt : supported) {
            combo->add_item(fmt.id, fmt.name);
            if (first) {
                combo->set_selected_item_by_id(fmt.id);
                first = false;
            }
        }
        m_format_combo = combo.get();

        form_container->add_child(std::move(format_label));
        form_container->add_child(std::move(combo));
        add_child(std::move(form_container));

        auto name_container = std::make_unique<Widget>();
        name_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        name_container->set_spacing(5);
        name_container->set_fixed_size(30);

        auto name_label = std::make_unique<Label>(horizon::i18n().tr("disk_utility.erase.label_name"));
        name_label->set_fixed_size(120);

        auto entry = std::make_unique<TextBox<horizon::TextPolicy>>();
        entry->set_placeholder(horizon::i18n().tr("disk_utility.erase.placeholder_name"));
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
        erase_btn->set_text(horizon::i18n().tr("disk_utility.erase.btn_erase"));
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
            m_status_label->set_text(horizon::i18n().tr("disk_utility.erase.status_selected_partition") + m_selected_partition->device_path + " (" + m_selected_partition->human_capacity() + ")");
            m_name_entry->set_text(m_selected_partition->label);
        } else if (m_selected_disk) {
            m_status_label->set_text(horizon::i18n().tr("disk_utility.erase.status_selected_disk") + m_selected_disk->device_path + " (" + m_selected_disk->human_capacity() + ")");
            m_name_entry->set_text("");
        } else {
            m_status_label->set_text(horizon::i18n().tr("disk_utility.erase.selection_hint"));
        }
    }

    void EraseTab::on_erase_clicked()
    {
        if (!m_selected_disk && !m_selected_partition) {
            application()->alert(horizon::i18n().tr("disk_utility.errors.select_for_erase"), horizon::i18n().tr("disk_utility.tabs.erase"));
            return;
        }

        auto selected_format = m_format_combo->selected_item();
        if (!selected_format) {
            application()->alert(horizon::i18n().tr("disk_utility.errors.invalid_format"), horizon::i18n().tr("disk_utility.tabs.erase"));
            return;
        }

        // --- Mount Validation ---
        if (m_selected_partition) {
            if (m_selected_partition->is_mounted) {
                application()->alert(horizon::i18n().tr("disk_utility.errors.mounted_partition"), horizon::i18n().tr("disk_utility.tabs.erase"));
                return;
            }
        } else if (m_selected_disk) {
            bool disk_mounted = false;
            for (const auto& part : m_selected_disk->partitions) {
                if (part->is_mounted) {
                    disk_mounted = true;
                    break;
                }
            }
            if (disk_mounted) {
                application()->alert(horizon::i18n().tr("disk_utility.errors.mounted_disk"), horizon::i18n().tr("disk_utility.tabs.erase"));
                return;
            }
        }

        std::string target_name = m_selected_partition ? m_selected_partition->device_path : m_selected_disk->device_path;
        bool is_internal = m_selected_disk ? !m_selected_disk->is_removable : true;

        horizon::Params params;
        params["device"] = target_name;
        std::string confirm_msg = horizon::i18n().tr("disk_utility.erase.confirm_device", params) + horizon::i18n().tr("disk_utility.erase.confirm_loss");
        if (is_internal) {
            confirm_msg = horizon::i18n().tr("disk_utility.erase.confirm_internal") + confirm_msg + horizon::i18n().tr("disk_utility.erase.confirm_sudo");
        }

        if (!application()->confirm(confirm_msg, horizon::i18n().tr("disk_utility.erase.confirm_title"))) {
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
        auto progress_dialog = std::make_shared<DiskProgressDialog>(horizon::i18n().tr("disk_utility.erase.btn_erase"), horizon::i18n().tr("disk_utility.operations.preparing"));
        
        // 2. Launch the dialog event loop in its own thread
        std::thread([dialog = progress_dialog]() {
            dialog->initialize();
            dialog->run();
        }).detach();

        // 3. Launch the worker thread for the disk operation
        std::thread([this, name, fs_type, device_path, is_partition, dialog = progress_dialog]() {
            // Set progress callback to update dialog
            this->m_disk_manager.set_progress_callback([dialog](float percent, const std::string& operation) {
                dialog->post_task([dialog, percent, operation]() {
                    dialog->set_progress(percent, operation);
                });
            });

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
                    update_status(horizon::i18n().tr("disk_utility.operations.partition_delete"));
                    result = this->m_disk_manager.recreate_and_format_partition(
                        device_path, fs_type, name);
                } else {
                    update_status(horizon::i18n().tr("disk_utility.operations.preparing_gpt"));
                    result = this->m_disk_manager.erase_disk(
                        device_path, fs_type, name);
                }
            } catch (const std::exception& e) {
                result = {false, e.what()};
            }

            // Clear progress callback
            this->m_disk_manager.set_progress_callback(nullptr);

            // Return to main thread via the APPLICATION'S task queue to avoid Segfault
            this->application()->post_task([this, result, dialog]() mutable {
                dialog->quit(); // This will exit the dialog's run() loop
                
                if (result.success) {
                    this->application()->alert(horizon::i18n().tr("disk_utility.operations.success"), horizon::i18n().tr("disk_utility.tabs.erase"));
                    
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
                    this->application()->alert(horizon::i18n().tr("disk_utility.operations.error_prefix") + result.message, horizon::i18n().tr("disk_utility.tabs.erase"), MessageType::Error);
                }
            });
        }).detach();
    }

} // namespace horizon::disks
