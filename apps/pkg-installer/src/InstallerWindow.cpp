#include "InstallerWindow.hpp"
#include <horizon/I18n.hpp>
#include "horizon/Spacer.hpp"
#include <filesystem>
#include <horizon/Application.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/Logger.hpp>
#include <thread>

namespace horizon
{

    InstallerWindow::InstallerWindow() : Window("Package Installer")
    {
        set_size(700, 550);
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
        icon_container->set_fixed_size(200);
        icon_container->set_margin(10);
        icon_container->set_spacing(40);
        icon_container->set_position_type(FILL);

        // App Icon Container (Source)
        auto app_icon_box = std::make_unique<Widget>();
        app_icon_box->set_fixed_size(128);
        app_icon_box->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        // Theme Icon
        auto app_icon_ptr = std::make_unique<Icon>();
        m_app_icon = app_icon_ptr.get();
        m_app_icon->set_icon_size(128);
        m_app_icon->set_icon_name("system-software-install");
        app_icon_box->add_child(std::move(app_icon_ptr));

        // File Image
        auto app_image_ptr = std::make_unique<Image>();
        m_app_image = app_image_ptr.get();
        m_app_image->set_fixed_size(128);
        m_app_image->set_mode(ImageMode::Fit);
        m_app_image->set_visible(false);
        app_icon_box->add_child(std::move(app_image_ptr));

        app_icon_box->set_draggable(true);
        app_icon_box->when_drag_start.connect(
            [this, app_icon_box_ptr = app_icon_box.get()](DragEventContext &ctx)
            {
                if (!m_current_deb)
                    return;

                std::vector<std::string> mimes = {"application/x-horizon-installer"};
                auto fetcher = [this](const std::string &mime) -> std::vector<uint8_t>
                { return std::vector<uint8_t>(m_deb_path.begin(), m_deb_path.end()); };
                application()->start_drag(mimes, fetcher, app_icon_box_ptr);
            });

        // System Icon (Target)
        auto sys_icon_ptr = std::make_unique<Icon>();
        m_system_icon = sys_icon_ptr.get();
        m_system_icon->set_icon_size(128);
        m_system_icon->set_fixed_size(128);
        m_system_icon->set_icon_name("folder-templates");

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

        icon_container->add_child(Spacer(10));
        icon_container->add_child(std::move(app_icon_box));
        icon_container->add_child(Spacer());

        // Arrow Icon (Indicator)
        auto arrow_icon = std::make_unique<Icon>();
        arrow_icon->set_icon_name("go-next");
        arrow_icon->set_icon_size(48);
        arrow_icon->set_fixed_size(48);
        arrow_icon->set_vertical_alignment(VerticalAlignment::Middle);
        icon_container->add_child(std::move(arrow_icon));

        icon_container->add_child(Spacer());
        icon_container->add_child(std::move(sys_icon_ptr));
        icon_container->add_child(Spacer(10));

        container->add_child(std::move(icon_container));

        // --- Package Info Section ---
        auto name_ptr = std::make_unique<Label>("");
        name_ptr->set_alignment(TextAlignment::Center);
        name_ptr->set_font_weight(FONT_WEIGHT_BOLD);
        name_ptr->set_font_size(24);
        name_ptr->set_fixed_size(50);
        m_name_label = name_ptr.get();
        container->add_child(std::move(name_ptr));

        auto desc_ptr = std::make_unique<Label>("");
        desc_ptr->set_alignment(TextAlignment::Center);
        m_desc_label = desc_ptr.get();
        container->add_child(std::move(desc_ptr));

        // --- Bottom Section: Feedback ---
        auto feedback_ptr =
            std::make_unique<Label>(i18n().tr("installer.drag_instruction"));
        feedback_ptr->set_alignment(TextAlignment::Center);
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
            update_status(i18n().tr("installer.error_read"), WidgetAccentColor::Error);
            return;
        }

        m_current_deb = info;
        m_deb_path = path;

        if (info->icon_path.empty())
        {
            m_app_icon->set_visible(true);
            m_app_image->set_visible(false);
            m_app_icon->set_icon_name("system-software-install");
        }
        else if (info->icon_is_theme_name)
        {
            m_app_icon->set_visible(true);
            m_app_image->set_visible(false);
            m_app_icon->set_icon_name(info->icon_path);
        }
        else
        {
            m_app_icon->set_visible(false);
            m_app_image->set_visible(true);
            m_app_image->set_path(info->icon_path);
        }

        set_title(i18n().tr("installer.ready") + " " + info->package_name);
        m_name_label->set_text(info->package_name);

        std::string desc = info->description;

        m_desc_label->set_text(info->version + " - " + desc);

        std::string msg = i18n().tr("installer.ready_msg", {{"name", info->package_name}, {"version", info->version}});
        if (info->is_installed)
        {
            msg = i18n().tr("installer.already_installed_msg", {{"name", info->package_name}});
        }
        update_status(msg);
    }

    void InstallerWindow::start_installation()
    {
        if (!m_current_deb)
            return;

        update_status(i18n().tr("installer.installing_msg", {{"name", m_current_deb->package_name}}));
        m_loading_bar->set_visible(true);

        std::string cmd = "pkexec env DEBIAN_FRONTEND=noninteractive apt-get install -y --reinstall \"" + m_deb_path + "\"";

        std::thread(
            [this, cmd]()
            {
                int result = std::system(cmd.c_str());

                if (result == 0)
                {
                    application()->post_task(
                        [this]()
                        {
                            update_status(i18n().tr("installer.success"),
                                          WidgetAccentColor::Success);
                            m_loading_bar->set_visible(false);
                        });
                }
                else
                {
                    application()->post_task(
                        [this]()
                        {
                            update_status(
                                i18n().tr("installer.failed"),
                                WidgetAccentColor::Error);
                            m_loading_bar->set_visible(false);
                        });
                }
            })
            .detach();
    }

    void InstallerWindow::update_status(const std::string &message, WidgetAccentColor accent)
    {
        m_feedback_label->set_text(message);
        
        if (accent == WidgetAccentColor::Success)
        {
            m_feedback_label->set_text_color(Color(0.1f, 0.7f, 0.1f, 1.0f)); // Verde
        }
        else if (accent == WidgetAccentColor::Error)
        {
            m_feedback_label->set_text_color(Color(0.9f, 0.1f, 0.1f, 1.0f)); // Rojo
        }
        else
        {
            m_feedback_label->set_text_color(Color(0.0f, 0.0f, 0.0f, 1.0f)); // Negro
        }
    }

} // namespace horizon
