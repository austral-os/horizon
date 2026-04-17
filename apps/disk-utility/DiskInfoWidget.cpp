#include "DiskInfoWidget.hpp"

namespace horizon::disks
{
    DiskInfoWidget::DiskInfoWidget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_margin(15);
        set_spacing(60);

        auto left_col = std::make_unique<Widget>();
        left_col->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_col->set_spacing(5);
        
        m_mount_point_lbl = create_info_row(left_col.get(), "Punto de montaje:");
        m_format_lbl = create_info_row(left_col.get(), "Formato:");
        
        auto right_col = std::make_unique<Widget>();
        right_col->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        right_col->set_spacing(5);
        
        m_capacity_lbl = create_info_row(right_col.get(), "Capacidad:");
        m_available_lbl = create_info_row(right_col.get(), "Disponible:");
        m_used_lbl = create_info_row(right_col.get(), "Espacio utilizado:");

        add_child(std::move(left_col));
        add_child(std::move(right_col));
    }

    Label* DiskInfoWidget::create_info_row(Widget* parent, const std::string& title)
    {
        auto row = std::make_unique<Widget>();
        row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        row->set_spacing(5);

        auto title_lbl = std::make_unique<Label>(title);
        title_lbl->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        // Ajustar el tamaño del título para alineación uniforme si es necesario
        title_lbl->set_fixed_size(140); 
        row->add_child(std::move(title_lbl));

        auto value_lbl = std::make_unique<Label>("-");
        auto* value_ptr = value_lbl.get();
        row->add_child(std::move(value_lbl));

        parent->add_child(std::move(row));
        return value_ptr;
    }

    void DiskInfoWidget::update_info(const DiskPartition& partition)
    {
        m_mount_point_lbl->set_text(partition.is_mounted ? partition.mount_point : "No montado");
        m_format_lbl->set_text(partition.filesystem);
        m_capacity_lbl->set_text(partition.human_capacity());
        m_used_lbl->set_text(partition.is_mounted ? partition.human_used() : "-");
        m_available_lbl->set_text(partition.is_mounted ? partition.human_available() : "No disponible");
    }

    void DiskInfoWidget::update_info(const DiskDevice& device)
    {
        m_mount_point_lbl->set_text(device.device_path);
        m_format_lbl->set_text(device.model);
        m_capacity_lbl->set_text(device.human_capacity());
        m_available_lbl->set_text(std::to_string(device.partitions.size()) + " particiones");
        m_used_lbl->set_text("Conectado");
    }

} // namespace horizon::disks
