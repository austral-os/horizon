#include "TopPanelApplication.hpp"
#include "TopPanelWidget.hpp"
#include "IndicatorsContainer.hpp"
#include "DateAndTimeIndicator.hpp"
#include "NetworkIndicator.hpp"
#include "BatteryIndicator.hpp"
#include "NotificationIndicator.hpp"
#include <horizon/DesktopEntry.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Widget.hpp>
#include <horizon/I18n.hpp>
#include <nlohmann/json.hpp>

using namespace horizon;

const int PANEL_HEIGHT = 32;

TopPanelApplication::TopPanelApplication()
    : Application("org.horizon.top_panel", 800, 32, true, true)
{
    m_window = create_layer_window("top_panel", 2); // 2 = ZWLR_LAYER_SHELL_V1_LAYER_TOP
    m_window->set_name("Top Panel");
    m_window->set_show_in_dock(false);

    // Setup About info
    auto &about = about_manager();
    about.set_app_title("Top Panel");
    about.set_app_description("Horizon Top Panel provides system indicators and application menus.");
    about.set_app_version("0.1.0");
    about.set_app_icon("preferences-system");
    about.set_app_git(ABOUT_HORIZON.git);

    DesktopEntry::add_search_path(
        "/home/horacio/Desarrollo/austral-os/horizon/examples/config/apps/");

    // Load translations
    i18n().load_app_locales("top_panel");

    setup_window();
    setup_ipc();

    auto root = std::make_unique<Widget>();
    root->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

    auto panel_widget = std::make_unique<TopPanelWidget>(this);
    m_root_widget = panel_widget.get();

    // Add initial indicators
    m_root_widget->indicators()->add_indicator(std::make_unique<BatteryIndicator>());
    m_root_widget->indicators()->add_indicator(std::make_unique<NetworkIndicator>());
    m_root_widget->indicators()->add_indicator(std::make_unique<NotificationIndicator>());
    m_root_widget->indicators()->add_indicator(std::make_unique<DateAndTimeIndicator>());

    root->add_child(std::move(panel_widget));
    m_window->set_root(std::move(root));
}

TopPanelApplication::~TopPanelApplication() = default;

void TopPanelApplication::setup_window()
{
    m_window->set_anchor(1 | 4 | 8); // TOP | LEFT | RIGHT
    m_window->set_size(0, PANEL_HEIGHT);
    m_window->set_exclusive_zone(PANEL_HEIGHT);
    m_window->set_keyboard_interactivity(0); // NONE
    m_window->set_visible(true);
}

void TopPanelApplication::setup_ipc()
{
    m_ipc_client = std::make_unique<IpcClient>("/tmp/horizon_session.sock");
    m_ipc_client->subscribe("{\"type\": \"subscribe\"}",
                            [this](const std::string &msg)
                            {
                                try
                                {
                                    auto j = nlohmann::json::parse(msg);
                                    if (j.value("receiver_id", "") == "top_panel")
                                    {
                                        std::lock_guard<std::mutex> lock(m_queue_mutex);
                                        m_pending_messages.push_back(msg);
                                    }
                                }
                                catch (...)
                                {
                                }
                            });

    // Timer to process messages
    add_timer(50, [this]() { process_messages(); }, true);
}

void TopPanelApplication::process_messages()
{
    std::vector<std::string> to_process;
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        if (m_pending_messages.empty())
            return;
        to_process = std::move(m_pending_messages);
        m_pending_messages.clear();
    }

    for (const auto &msg : to_process)
    {
        m_root_widget->handle_message(msg);
    }
}

void TopPanelApplication::run_panel()
{
    LOG_INFO << "Top Panel started (32px).";
    run();
}
