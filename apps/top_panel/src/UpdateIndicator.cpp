#include "UpdateIndicator.hpp"
#include <array>
#include <chrono>
#include <cstdio>
#include <horizon/Application.hpp>
#include <horizon/Notification.hpp>
#include <memory>
#include <string>

namespace horizon
{

    UpdateIndicator::UpdateIndicator() : ITopPanelWidget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(FILL);

        m_icon = std::make_unique<Icon>();
        m_icon->set_icon_name("safety-symbolic");
        m_icon->set_icon_size(20);
        m_icon->set_fixed_size(24);
        m_icon->set_vertical_alignment(VerticalAlignment::Middle);
        m_icon->set_use_theme_colors(true);
        m_icon->set_theme_color_key("window_fg");
        add_child(std::move(m_icon));

        auto tip = std::make_unique<Notification>();
        tip->set_notification("safety-symbolic", "Buscando actualizaciones...");
        set_tooltip(std::move(tip));

        this->when_click.connect(
            [this](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 0x110)
                { // Left click
                    std::system("horizon-appstore --updates &");
                }
            });

        m_running = true;
        m_monitor_thread = std::thread(
            [this]()
            {
                // First check almost immediately
                std::this_thread::sleep_for(std::chrono::seconds(2));
                if (!m_running)
                    return;

                int count = check_updates();
                if (application())
                {
                    application()->post_task([this, count]() { update_ui(count); });
                }

                while (m_running)
                {
                    // Wait for 30 minutes (1800 seconds) in small intervals to allow quick exit
                    for (int i = 0; i < 1800; ++i)
                    {
                        if (!m_running)
                            return;
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }

                    int current_count = check_updates();
                    if (application())
                    {
                        application()->post_task([this, current_count]()
                                                 { update_ui(current_count); });
                    }
                }
            });
    }

    UpdateIndicator::~UpdateIndicator()
    {
        m_running = false;
        if (m_monitor_thread.joinable())
        {
            m_monitor_thread.join();
        }
    }

    int UpdateIndicator::preferred_width() const
    {
        return 24 + (margin() * 2);
    }

    int UpdateIndicator::check_updates()
    {
        int count = 0;
        // apt-get -s upgrade | grep -c "^Inst "
        // This command counts the number of lines starting with "Inst " which represents packages
        // to be installed/upgraded.
        std::unique_ptr<FILE, decltype(&pclose)> pipe(
            popen("apt-get -s upgrade 2>/dev/null | grep -c \"^Inst \"", "r"), pclose);
        if (!pipe)
        {
            return 0;
        }

        std::array<char, 128> buffer;
        std::string result;
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }

        try
        {
            count = std::stoi(result);
        }
        catch (...)
        {
            count = 0;
        }

        return count;
    }

    void UpdateIndicator::update_ui(int update_count)
    {
        auto icon_widget = static_cast<Icon *>(children()[0].get());
        auto tip = std::make_unique<Notification>();
        if (update_count > 0)
        {
            icon_widget->set_icon_name("software-update-available-symbolic");
            tip->set_notification("software-update-available-symbolic",
                                  std::to_string(update_count) + " actualizaciones disponibles");
        }
        else
        {
            icon_widget->set_icon_name("safety-symbolic");
            tip->set_notification("safety-symbolic", "Sistema actualizado");
        }
        set_tooltip(std::move(tip));
    }

} // namespace horizon
