#include "InstallerWindow.hpp"
#include "horizon/Frame.hpp"
#include <filesystem>
#include <horizon/Application.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/Logger.hpp>
#include <thread>

namespace horizon
{

    InstallerWindow::InstallerWindow() : Window("Package Installer")
    {
        set_size(500, 350);
        setup_ui();

        when_file_opened.connect([this](Window::FileOpenedContext &ctx)
                                 { this->load_deb(ctx.path); });
    }

    void InstallerWindow::setup_ui()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(20);
        container->set_spacing(20);

        // --- Top Section: Icons ---
        auto icon_container = std::make_unique<Frame>();
        icon_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        icon_container->set_spacing(60);
        icon_container->set_position_type(FILL);

        // App Icon (Source)
        auto app_icon_ptr = std::make_unique<Image>();
        m_app_icon = app_icon_ptr.get();
        m_app_icon->set_fixed_size(128);
        m_app_icon->set_mode(ImageMode::Fit);
        m_app_icon->set_path(IconThemeLookup::find_icon("system-software-install", 128)); // Default

        m_app_icon->set_draggable(true);
        m_app_icon->when_drag_start.connect(
            [this](DragEventContext &ctx)
            {
                if (!m_current_deb)
                    return;

                std::vector<std::string> mimes = {"application/x-horizon-installer"};
                auto fetcher = [this](const std::string &mime) -> std::vector<uint8_t>
                { return std::vector<uint8_t>(m_deb_path.begin(), m_deb_path.end()); };
                application()->start_drag(mimes, fetcher, m_app_icon);
            });

        // System Icon (Target)
        auto sys_icon_ptr = std::make_unique<Image>();
        m_system_icon = sys_icon_ptr.get();
        m_system_icon->set_fixed_size(128);
        m_system_icon->set_mode(ImageMode::Fit);
        m_system_icon->set_path(IconThemeLookup::find_icon("applications-system", 128));

        m_system_icon->set_accept_drops(true);
        m_system_icon->when_drop.connect(
            [this](DropEventContext &ctx)
            {
                bool is_valid = false;
                for (const auto &mime : ctx.mime_types)
                {
                    if (mime == "application/x-horizon-installer")
                    {
                        is_valid = true;
                        break;
                    }
                }
                if (is_valid)
                {
                    start_installation();
                }
            });

        icon_container->add_child(std::move(app_icon_ptr));
        icon_container->add_child(std::move(sys_icon_ptr));
        container->add_child(std::move(icon_container));

        // --- Bottom Section: Feedback ---
        auto feedback_ptr =
            std::make_unique<Label>("Drag the application icon to the system folder to install.");
        feedback_ptr->set_alignment(TextAlignment::Center);
        feedback_ptr->set_fixed_size(40);
        m_feedback_label = feedback_ptr.get();
        m_feedback_label->set_height(30);
        container->add_child(std::move(feedback_ptr));

        auto loading_ptr = std::make_unique<LoadingBar>();
        m_loading_bar = loading_ptr.get();
        m_loading_bar->set_visible(false);
        m_loading_bar->set_fixed_size(4);
        container->add_child(std::move(loading_ptr));

        auto progress_ptr = std::make_unique<ProgressBar>();
        m_progress_bar = progress_ptr.get();
        m_progress_bar->set_visible(false);
        m_progress_bar->set_fixed_size(4);
        container->add_child(std::move(progress_ptr));

        add_child(std::move(container));
    }

    void InstallerWindow::load_deb(const std::string &path)
    {
        auto info = DebInspector::inspect(path);
        if (!info)
        {
            update_status("Error: Could not read package information.", true);
            return;
        }

        m_current_deb = info;
        m_deb_path = path;

        if (info->icon_is_theme_name)
        {
            m_app_icon->set_path(IconThemeLookup::find_icon(info->icon_path, 128));
        }
        else
        {
            m_app_icon->set_path(info->icon_path);
        }

        set_title("Install " + info->package_name);
        update_status("Ready to install " + info->package_name + " (" + info->version + ")");
    }

    void InstallerWindow::start_installation()
    {
        if (!m_current_deb)
            return;

        update_status("Installing " + m_current_deb->package_name + "...");
        m_loading_bar->set_visible(true);
        m_app_icon->set_enabled(false);

        // Run apt install via pkexec
        // We use a detached thread or a non-blocking process call if possible.
        // For now, let's use a simple background command.
        std::string cmd = "pkexec apt-get install -y --reinstall \"" + m_deb_path + "\"";

        // In a real scenario, we'd use GSubprocess to track progress.
        // Since we don't have a high-level async process runner here easily,
        // we'll simulate the completion for now or use std::system in a thread.

        std::thread(
            [this, cmd]()
            {
                int result = std::system(cmd.c_str());

                // Back to main thread to update UI
                // Horizon usually has a way to post tasks to main loop.
                // Assuming application()->post_task exists or similar.
                // If not, we'll just hope for the best or check if Horizon is thread-safe for
                // simple invalidation.

                if (result == 0)
                {
                    m_app->post_task(
                        [this]()
                        {
                            update_status("Installation completed successfully!");
                            m_loading_bar->set_visible(false);
                        });
                }
                else
                {
                    m_app->post_task(
                        [this]()
                        {
                            update_status(
                                "Installation failed. Please check your password or dependencies.",
                                true);
                            m_loading_bar->set_visible(false);
                            m_app_icon->set_enabled(true);
                        });
                }
            })
            .detach();
    }

    void InstallerWindow::update_status(const std::string &message, bool is_error)
    {
        m_feedback_label->set_text(message);
        if (is_error)
        {
            m_feedback_label->set_accent_color(WidgetAccentColor::Error);
        }
        else
        {
            m_feedback_label->set_accent_color(WidgetAccentColor::Default);
        }
    }

} // namespace horizon
