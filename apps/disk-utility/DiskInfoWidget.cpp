#include "DiskInfoWidget.hpp"

namespace horizon::disks
{
    DiskInfoWidget::DiskInfoWidget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_margin(15);
        set_spacing(40);

        auto left_col = std::make_unique<Widget>();
        left_col->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_col->set_spacing(5);
        
        auto mount_lbl = std::make_unique<Label>("Punto de montaje: -");
        m_mount_point_lbl = mount_lbl.get();
        left_col->add_child(std::move(mount_lbl));
        
        auto format_lbl = std::make_unique<Label>("Formato: -");
        m_format_lbl = format_lbl.get();
        left_col->add_child(std::move(format_lbl));
        
        auto right_col = std::make_unique<Widget>();
        right_col->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        right_col->set_spacing(5);
        
        auto capacity_lbl = std::make_unique<Label>("Capacidad: -");
        m_capacity_lbl = capacity_lbl.get();
        right_col->add_child(std::move(capacity_lbl));
        
        auto available_lbl = std::make_unique<Label>("Disponible: -");
        m_available_lbl = available_lbl.get();
        right_col->add_child(std::move(available_lbl));
        
        auto used_lbl = std::make_unique<Label>("Espacio utilizado: -");
        m_used_lbl = used_lbl.get();
        right_col->add_child(std::move(used_lbl));

        add_child(std::move(left_col));
        add_child(std::move(right_col));
    }

    void DiskInfoWidget::update_info(const DiskPartition& partition)
    {
        m_mount_point_lbl->set_text("Punto de montaje: " + (partition.is_mounted ? partition.mount_point : "No montado"));
        m_format_lbl->set_text("Formato: " + partition.filesystem);
        m_capacity_lbl->set_text("Capacidad: " + partition.human_capacity());
        m_used_lbl->set_text("Espacio utilizado: " + partition.human_used());
        
        uint64_t available = partition.capacity - partition.used;
        // Convert to human available size (simplified)
        m_available_lbl->set_text("Disponible: (calculando...)"); 
    }

    void DiskInfoWidget::update_info(const DiskDevice& device)
    {
        m_mount_point_lbl->set_text("Dispositivo: " + device.device_path);
        m_format_lbl->set_text("Modelo: " + device.model);
        m_capacity_lbl->set_text("Capacidad: " + device.human_capacity());
        m_available_lbl->set_text("Particiones: " + std::to_string(device.partitions.size()));
        m_used_lbl->set_text("Estado: Conectado");
    }

} // namespace horizon::disks
