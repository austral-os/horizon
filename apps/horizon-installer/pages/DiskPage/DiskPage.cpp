#include "DiskPage.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Label.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/TreeViewItem.hpp>
#include <horizon-disk-utilities/DiskManager.hpp>

namespace horizon::installer
{
    DiskPage::DiskPage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto title = std::make_unique<Label>(i18n().tr("installer.disk.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.disk.desc"));
        desc->set_alignment(TextAlignment::Center);
        add_child(std::move(desc));

        add_child(Spacer(20));

        auto disk_tree = std::make_unique<TreeView>();
        m_disk_tree = disk_tree.get();
        add_child(std::move(disk_tree));
        m_disk_tree->set_size(500, 250);
        
        refresh_disks();

        add_child(Spacer());

        auto btn_back = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.back"), "go-previous");
        btn_back->when_click.connect([this](auto&) { 
            EventContext ctx;
            when_back.run(ctx); 
        });

        auto btn_install = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.install"), "system-run");
        btn_install->when_click.connect([this](auto&) {
            if (m_disk_tree->selected_item()) {
                selected_device_str = m_disk_tree->selected_item()->get_text();
                EventContext ctx;
                when_install.run(ctx);
            }
        });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_back));
        btn_container->add_child(Spacer(20));
        btn_container->add_child(std::move(btn_install));
        btn_container->add_child(Spacer());
        add_child(std::move(btn_container));
    }

    void DiskPage::refresh_disks()
    {
        m_disk_tree->clear_root_items();
        horizon::disks::DiskManager manager;
        manager.scan();
        
        for (const auto& dev : manager.devices()) {
            std::string label = dev->name + " (" + dev->human_capacity() + ")";
            auto item = std::make_unique<TreeViewItem>("drive-harddisk", label);
            m_disk_tree->add_root_item(std::move(item));
        }
    }
} // namespace horizon::installer
