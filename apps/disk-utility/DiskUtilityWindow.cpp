#include "DiskUtilityWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/Label.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Widget.hpp>

namespace horizon::disks
{
    DiskUtilityWindow::DiskUtilityWindow() : ApplicationWindow("Utilidad de Discos")
    {
        set_size(960, 700);
        setup_toolbar();
        setup_layout();

        m_disk_manager.scan();
        populate_devices();
    }

    void DiskUtilityWindow::setup_toolbar()
    {
        auto tb = toolbar();
        tb->set_bottom_height(58);

        auto info_btn = std::make_unique<ToolbarButton>("Información", "info");
        auto mount_btn = std::make_unique<ToolbarButton>("Montar", "media-mount");
        auto eject_btn = std::make_unique<ToolbarButton>("Expulsar", "media-eject");
        auto new_img_btn = std::make_unique<ToolbarButton>("Nueva Imagen", "document-new");

        tb->add_toolbar_widget(std::move(info_btn));
        tb->add_toolbar_widget(std::move(mount_btn));
        tb->add_toolbar_widget(std::move(eject_btn));
        tb->add_toolbar_widget(std::move(new_img_btn));
    }

    void DiskUtilityWindow::setup_layout()
    {
        auto main_container = std::make_unique<Widget>();
        main_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        main_container->set_position_type(WidgetPositionTypes::FILL);

        // --- Top Area (Splitted with VPanel) ---
        auto top_area = std::make_unique<VPanel>();
        top_area->set_left_width(240);
        top_area->set_position_type(WidgetPositionTypes::FILL);

        // Sidebar (Left side of VPanel)
        auto tree = std::make_unique<TreeView>();
        m_device_tree = tree.get();
        top_area->add_child(std::move(tree));

        // Notebook (Right side of VPanel)
        auto notebook = std::make_unique<Notebook>();
        m_notebook = notebook.get();

        // Tab: Borrar
        auto erase_tab = std::make_unique<EraseTab>(m_disk_manager);
        m_erase_tab = erase_tab.get();
        m_notebook->add_tab({"Borrar", std::move(erase_tab)});

        auto partition_body = std::make_unique<Widget>();
        partition_body->add_child(std::make_unique<Label>("Contenido de la pestaña Particiones"));
        m_notebook->add_tab({"Particiones", std::move(partition_body)});

        auto restore_body = std::make_unique<Widget>();
        restore_body->add_child(std::make_unique<Label>("Contenido de la pestaña Restaurar"));
        m_notebook->add_tab({"Restaurar", std::move(restore_body)});

        top_area->add_child(std::move(notebook));

        // --- Info Panel (Bottom, Full Width) ---
        auto info_panel = std::make_unique<DiskInfoWidget>();
        info_panel->set_fixed_size(120);
        // info_panel->set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
        info_panel->set_border_width(1);
        info_panel->set_border_color(Color(0.85f, 0.85f, 0.85f, 1.0f));
        m_info_panel = info_panel.get();

        main_container->add_child(std::move(top_area));
        main_container->add_child(std::move(info_panel));

        set_content(std::move(main_container));
    }

    void DiskUtilityWindow::populate_devices()
    {
        if (!m_device_tree)
            return;
        m_device_tree->clear_root_items();

        for (const auto &disk : m_disk_manager.devices())
        {
            auto disk_item =
                std::make_unique<TreeViewItem>("drive-harddisk", disk->full_model_name());

            for (const auto &part : disk->partitions)
            {
                auto part_item = std::make_unique<TreeViewItem>(
                    "partition", part->name + " (" + part->human_capacity() + ")");
                disk_item->add_child(std::move(part_item));
            }

            m_device_tree->add_root_item(std::move(disk_item));
        }

        m_device_tree->when_item_selected.connect([this](TreeViewItem *item)
                                                  { this->on_item_selected(item); });
    }

    void DiskUtilityWindow::on_item_selected(TreeViewItem *item)
    {
        if (!item)
            return;

        // Simplify: Search in DiskManager
        for (const auto &disk : m_disk_manager.devices())
        {
            if (disk->full_model_name() == item->get_text())
            {
                m_info_panel->update_info(*disk);
                return;
            }

            for (const auto &part : disk->partitions)
            {
                if ((part->name + " (" + part->human_capacity() + ")") == item->get_text())
                {
                    m_info_panel->update_info(*part);
                    m_erase_tab->set_selected_partition(part->device_path);
                    return;
                }
            }
        }
    }

} // namespace horizon::disks
