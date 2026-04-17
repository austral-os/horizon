#include "DiskInfoDialog.hpp"
#include <horizon/DiskInfoWidget.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Window.hpp>

namespace horizon::disks
{
    DiskInfoDialog::DiskInfoDialog(const DiskInfo &info)
        : WaylandWindow("org.horizon.disk-utility.info", 830, 320, true, false)
    {
        set_name(horizon::i18n().tr("disk_utility.toolbar.info"));

        // Use Window as a widget container to get the titlebar
        auto win = std::make_unique<Window>(horizon::i18n().tr("disk_utility.toolbar.info"));
        win->add_child(std::make_unique<horizon::DiskInfoWidget>(info));

        set_root(std::move(win));
    }

} // namespace horizon::disks
