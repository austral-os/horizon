#include "DiskProgressDialog.hpp"
#include <horizon/Label.hpp>
#include <horizon/LoadingBar.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>
#include <horizon/Window.hpp>

namespace horizon::disks
{
    DiskProgressDialog::DiskProgressDialog(const std::string &title,
                                           const std::string &initial_status)
        : WaylandWindow("disk-utility.progress", 350, 160, true, false)
    {
        set_name(title);
        setup_ui(initial_status);
    }

    void DiskProgressDialog::setup_ui(const std::string &initial_status)
    {
        auto root = std::make_unique<Window>(name());
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        // root->set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f)); // Window handles background

        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(16);
        container->set_spacing(16);

        // --- LoadingBar ---
        auto progress = std::make_unique<LoadingBar>();
        m_loading_bar = progress.get();

        // --- Label: Status ---
        auto status_lbl = std::make_unique<Label>(initial_status);
        status_lbl->set_font_size(11);
        m_status_label = status_lbl.get();
        m_base_status = initial_status; // Store base text

        container->add_child(std::move(progress));
        container->add_child(std::move(status_lbl));

        root->add_child(std::move(container));

        set_root(std::move(root));
    }

    void DiskProgressDialog::set_status(const std::string &status)
    {
        m_base_status = status;
        if (m_status_label)
        {
            m_status_label->set_text(status);
            invalidate();
        }
    }

    void DiskProgressDialog::set_progress(float percent, const std::string& operation)
    {
        if (!operation.empty())
        {
            // Map technical operation names to user-friendly text
            std::string status = operation;
            if (operation == "partition-delete") status = "Borrando la partición anterior...";
            else if (operation == "create-partition") status = "Creando la nueva partición...";
            else if (operation == "create-filesystem" || operation == "format-mkfs") status = "Creando el sistema de archivos...";
            else if (operation == "erase-device") status = "Borrando el dispositivo dispositivo...";
            else if (operation == "ata-secure-erase") status = "Borrando de forma segura (ATA)...";
            
            m_base_status = status;
        }

        if (m_status_label)
        {
            m_status_label->set_text(m_base_status);
            invalidate();
        }
    }
} // namespace horizon::disks
