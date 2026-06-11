#include "CaptureWindow.hpp"
#include <chrono>
#include <horizon/Application.hpp>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuBar.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/NotificationSender.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/capture/SelectionWindow.h>
#include <horizon/SystemInfo.hpp>
#include <thread>

namespace horizon::capture
{

    CaptureWindow::CaptureWindow() : ApplicationWindow("Capture")
    {
        m_recorder = std::make_shared<VideoRecorder>();
        if (!m_engine.init())
        {
            LOG_ERROR << "[CaptureApp] Failed to initialize CaptureEngine!";
        }

        char *home = std::getenv("HOME");
        std::string config_path =
            home ? std::string(home) + "/.config/horizon/capture.json" : "capture.json";
        m_config = std::make_unique<ConfigManager>(config_path);

        when_application_load.connect(
            [this](EventContext &)
            {
                auto *app = application();
                if (!app)
                    return;

                LOG_INFO << "[CaptureApp] Setting up global menu signal connections";

                app->signal_manager.connect(
                    "img_selection",
                    [this](const SignalContext &)
                    {
                        LOG_INFO << "[CaptureApp] Global Signal: img_selection received";
                        this->execute_with_delay([this]() { this->capture_selection_image(); });
                    });
                app->signal_manager.connect(
                    "img_window",
                    [this](const SignalContext &)
                    {
                        LOG_INFO << "[CaptureApp] Global Signal: img_window received";
                        this->execute_with_delay([this]() { this->capture_window_image(); });
                    });
                app->signal_manager.connect(
                    "img_screen",
                    [this](const SignalContext &)
                    {
                        LOG_INFO << "[CaptureApp] Global Signal: img_screen received";
                        this->execute_with_delay([this]() { this->capture_screen_image(); });
                    });
                app->signal_manager.connect(
                    "vid_selection",
                    [this](const SignalContext &)
                    {
                        LOG_INFO << "[CaptureApp] Global Signal: vid_selection received";
                        this->execute_with_delay([this]() { this->start_selection_video(); });
                    });
                app->signal_manager.connect(
                    "vid_window",
                    [this](const SignalContext &)
                    {
                        LOG_INFO << "[CaptureApp] Global Signal: vid_window received";
                        this->execute_with_delay([this]() { this->start_window_video(); });
                    });
                app->signal_manager.connect(
                    "vid_screen",
                    [this](const SignalContext &)
                    {
                        LOG_INFO << "[CaptureApp] Global Signal: vid_screen received";
                        this->execute_with_delay([this]() { this->start_screen_video(); });
                    });
            });
        m_config->load();

        setup_ui();
    }

    CaptureWindow::~CaptureWindow()
    {
        if (m_is_recording)
        {
            stop_video();
        }
    }

    void CaptureWindow::setup_ui()
    {
        auto content_panel = std::make_unique<Widget>();
        content_panel->set_spacing(20);
        content_panel->set_margin(40);
        content_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto title = std::make_unique<Label>(horizon::i18n().tr("capture.title"));
        title->set_fixed_size(32);
        // title->set_bold(true); // If supported
        content_panel->add_child(std::move(title));

        auto subtitle = std::make_unique<Label>(horizon::i18n().tr("capture.about.description"));
        subtitle->set_fixed_size(25);
        content_panel->add_child(std::move(subtitle));

        auto delay_panel = std::make_unique<Widget>();
        delay_panel->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        delay_panel->set_fixed_size(35);
        delay_panel->set_spacing(10);
        auto delay_label = std::make_unique<Label>(horizon::i18n().tr("capture.delay.label"));
        delay_label->set_fixed_size(80);
        delay_panel->add_child(std::move(delay_label));

        auto delay_combo = std::make_unique<Combo>();
        delay_combo->set_fixed_size(150);
        m_delay_combo = delay_combo.get();
        m_delay_combo->add_item("0", horizon::i18n().tr("capture.timeout.none"));
        m_delay_combo->add_item("1", horizon::i18n().tr("capture.timeout.seconds_1"));
        m_delay_combo->add_item("2", horizon::i18n().tr("capture.timeout.seconds_2"));
        m_delay_combo->add_item("3", horizon::i18n().tr("capture.timeout.seconds_3"));
        m_delay_combo->add_item("4", horizon::i18n().tr("capture.timeout.seconds_4"));
        m_delay_combo->add_item("5", horizon::i18n().tr("capture.timeout.seconds_5"));
        m_delay_combo->add_item("10", horizon::i18n().tr("capture.timeout.seconds_10"));
        m_delay_combo->set_selected_item_index(0);
        m_delay_combo->when_item_selected.connect([this](const ComboItemSelectedContext &ctx)
                                                  { m_delay_seconds = std::stoi(ctx.item.id); });
        delay_panel->add_child(std::move(delay_combo));
        delay_panel->add_child(Spacer());
        content_panel->add_child(std::move(delay_panel));

        auto target_panel = std::make_unique<Widget>();
        target_panel->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        target_panel->set_fixed_size(35);
        target_panel->set_spacing(10);
        auto target_label = std::make_unique<Label>("Capturar:");
        target_label->set_fixed_size(80);
        target_panel->add_child(std::move(target_label));

        auto target_combo = std::make_unique<Combo>();
        target_combo->set_fixed_size(250);
        m_target_combo = target_combo.get();
        m_target_combo->add_item("area", "Área seleccionada");
        
        auto outputs = m_engine.get_all_outputs();
        int screen_idx = 1;
        for (const auto& o : outputs) {
            std::string text = "Pantalla " + std::to_string(screen_idx) + " (" + o.name + ")";
            m_target_combo->add_item("screen:" + o.name, text);
            screen_idx++;
        }
        
        m_target_combo->set_selected_item_index(0);
        target_panel->add_child(std::move(target_combo));
        target_panel->add_child(Spacer());
        content_panel->add_child(std::move(target_panel));

        m_status_label = new Label(horizon::i18n().tr("capture.status.ready"));
        m_status_label->set_fixed_size(25);
        auto status_ptr = std::unique_ptr<Label>(m_status_label);
        content_panel->add_child(std::move(status_ptr));

        // Add a spacer to push everything to the top
        content_panel->add_child(Spacer());

        // Toolbar setup
        m_toolbar = toolbar();
        if (m_toolbar)
        {
            m_toolbar->set_bottom_height(58);
            auto img_btn_ptr = std::make_unique<ToolbarButton>(
                horizon::i18n().tr("capture.toolbar.screenshot"), "camera-photo-symbolic");
            auto *img_btn = img_btn_ptr.get();
            m_toolbar->add_toolbar_widget(std::move(img_btn_ptr));
            img_btn->when_click.connect(
                [this](const MouseButtonEventContext &)
                { 
                    this->execute_with_delay([this]() {
                        if (m_target_combo) {
                            auto item = m_target_combo->selected_item();
                            if (item && item->id == "area") {
                                this->capture_selection_image();
                            } else if (item && item->id.find("screen:") == 0) {
                                std::string output_name = item->id.substr(7);
                                this->capture_screen_image(output_name);
                            } else {
                                this->capture_screen_image("");
                            }
                        } else {
                            this->capture_screen_image("");
                        }
                    }); 
                });

            auto vid_btn_ptr = std::make_unique<ToolbarButton>(
                horizon::i18n().tr("capture.toolbar.record"), "camera-video-symbolic");
            m_record_btn = vid_btn_ptr.get();
            m_toolbar->add_toolbar_widget(std::move(vid_btn_ptr));
            m_record_btn->when_click.connect(
                [this](const MouseButtonEventContext &)
                {
                    if (m_is_recording)
                        stop_video();
                    else
                        this->execute_with_delay([this]() {
                            if (m_target_combo) {
                                auto item = m_target_combo->selected_item();
                                if (item && item->id == "area") {
                                    this->start_selection_video();
                                } else if (item && item->id.find("screen:") == 0) {
                                    std::string output_name = item->id.substr(7);
                                    this->start_screen_video(output_name);
                                } else {
                                    this->start_screen_video("");
                                }
                            } else {
                                this->start_screen_video("");
                            }
                        });
                });

            auto pref_btn_ptr = std::make_unique<ToolbarButton>(
                horizon::i18n().tr("capture.toolbar.preferences"), "preferences-system-symbolic");
            auto *pref_btn = pref_btn_ptr.get();
            m_toolbar->add_toolbar_widget(std::move(pref_btn_ptr));
            pref_btn->when_click.connect(
                [this](const MouseButtonEventContext &)
                {
                    if (this->application())
                    {
                        this->application()->show_preferences();
                    }
                });
        }

        set_content(std::move(content_panel));
        set_size(600, 400);
    }

    void CaptureWindow::execute_with_delay(std::function<void()> action)
    {
        if (m_delay_seconds <= 0)
        {
            action();
            return;
        }

        if (m_countdown_timer_id != 0)
        {
            application()->stop_timer(m_countdown_timer_id);
        }

        m_current_countdown = m_delay_seconds;
        std::string text = horizon::i18n().tr("capture.status.countdown");

        // Replace {} with the number. Simple replace since std::string doesn't have it natively.
        size_t pos = text.find("{}");
        if (pos != std::string::npos)
        {
            text.replace(pos, 2, std::to_string(m_current_countdown));
        }
        else
        {
            text += " " + std::to_string(m_current_countdown);
        }
        m_status_label->set_text(text);

        m_countdown_timer_id = application()->add_timer(
            1000,
            [this, action]()
            {
                m_current_countdown--;
                if (m_current_countdown <= 0)
                {
                    application()->stop_timer(m_countdown_timer_id);
                    m_countdown_timer_id = 0;
                    m_status_label->set_text(horizon::i18n().tr("capture.status.ready"));
                    action();
                }
                else
                {
                    std::string t = horizon::i18n().tr("capture.status.countdown");
                    size_t p = t.find("{}");
                    if (p != std::string::npos)
                        t.replace(p, 2, std::to_string(m_current_countdown));
                    else
                        t += " " + std::to_string(m_current_countdown);
                    m_status_label->set_text(t);
                }
            },
            true);
    }

    std::string expand_tilde(std::string path)
    {
        if (path.empty() || path[0] != '~')
            return path;
        const char *home = std::getenv("HOME");
        if (!home)
            return path;
        if (path.size() == 1)
            return home;
        if (path[1] == '/')
            return std::string(home) + path.substr(1);
        return path;
    }

    void CaptureWindow::capture_screen_image(const std::string& output_name)
    {
        m_config->load(); // Reload in case preferences changed

        std::string out_dir = expand_tilde(m_config->get_value("general", "output_directory", "."));
        std::string format = m_config->get_value("image", "format", "png");

        // Ensure directory exists
        if (!out_dir.empty() && !std::filesystem::exists(out_dir))
        {
            std::filesystem::create_directories(out_dir);
        }

        LOG_INFO << "[CaptureApp] Starting screen capture...";
        m_status_label->set_text(horizon::i18n().tr("capture.status.capturing_screen"));

        std::string filename =
            "screenshot_" +
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." +
            format;
        std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;

        LOG_INFO << "[CaptureApp] Saving to: " << full_path.string();

        if (m_engine.capture_screenshot(output_name, full_path.string()))
        {
            LOG_INFO << "[CaptureApp] Screenshot successful";
            NotificationSender::send(
                "Captura realizada",
                "La imagen se ha guardado en: " + full_path.filename().string(), "camera-photo");
            m_status_label->set_text(
                horizon::i18n()
                    .tr("capture.status.saved")
                    .replace(horizon::i18n().tr("capture.status.saved").find("{}"), 2,
                             full_path.string()));
        }
        else
        {
            LOG_ERROR << "[CaptureApp] Screenshot failed";
            m_status_label->set_text(horizon::i18n().tr("capture.status.failed"));
        }
    }

    void CaptureWindow::capture_selection_image()
    {
        LOG_INFO << "[CaptureApp] capture_selection_image() called";
        m_status_label->set_text(horizon::i18n().tr("capture.status.select_region"));
        m_selection_win = std::make_shared<SelectionWindow>();

        std::thread(
            [win = m_selection_win]()
            {
                win->initialize();
                win->set_visible(true);
                win->run();
            })
            .detach();

        m_selection_win->selection_widget()->when_selected().connect(
            [this](SelectionRect rect)
            {
                int x = rect.x + m_selection_win->screen_x();
                int y = rect.y + m_selection_win->screen_y();
                int w = rect.width;
                int h = rect.height;

                std::string target_output = m_selection_win->w_surface()->current_output_name();
                if (target_output.empty()) {
                    LOG_ERROR << "[CaptureApp] Could not determine output name for selection window!";
                    auto outputs = m_engine.get_all_outputs();
                    if (!outputs.empty()) {
                        target_output = outputs[0].name;
                    }
                }

                LOG_INFO << "[CaptureApp] Capturing region from output: " << target_output 
                         << " local coords: x=" << x << " y=" << y << " w=" << w << " h=" << h;

                m_selection_win->set_visible(false);
                m_selection_win->quit();
                this->application()->post_task([this]() { m_selection_win.reset(); });

                // Small delay to ensure the window is gone from the compositor's view
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                m_config->load();
                std::string out_dir =
                    expand_tilde(m_config->get_value("general", "output_directory", "."));
                std::string format = m_config->get_value("image", "format", "png");

                if (!out_dir.empty() && !std::filesystem::exists(out_dir))
                {
                    std::filesystem::create_directories(out_dir);
                }

                std::string filename =
                    "screenshot_selection_" +
                    std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
                    "." + format;
                std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;

                if (m_engine.capture_region(target_output, x, y, w, h, full_path.string()))
                {
                    NotificationSender::send("Captura realizada",
                                             "La selección se ha guardado en: " +
                                                 full_path.filename().string(),
                                             "camera-photo");
                    m_status_label->set_text(
                        horizon::i18n()
                            .tr("capture.status.saved")
                            .replace(horizon::i18n().tr("capture.status.saved").find("{}"), 2,
                                     full_path.string()));
                }
                else
                {
                    m_status_label->set_text(horizon::i18n().tr("capture.status.failed"));
                }
            });

        m_selection_win->selection_widget()->when_cancelled().connect(
            [this](const EventContext &)
            {
                LOG_INFO << "[CaptureApp] Selection cancelled";
                m_selection_win->set_visible(false);
                m_selection_win->quit();
                this->application()->post_task([this]() { m_selection_win.reset(); });
                m_status_label->set_text(horizon::i18n().tr("capture.status.ready"));
            });
    }

    void CaptureWindow::capture_window_image()
    {
        m_status_label->set_text(horizon::i18n().tr("capture.status.not_implemented"));
        application()->alert(horizon::i18n().tr("capture.alert.not_implemented_msg"),
                             horizon::i18n().tr("capture.alert.not_implemented_title"));
    }

    void CaptureWindow::start_screen_video(const std::string& output_name)
    {
        if (m_is_recording)
            return;

        m_config->load();
        std::string out_dir = expand_tilde(m_config->get_value("general", "output_directory", "."));
        std::string container = m_config->get_value("video", "container", "mp4");

        // Ensure directory exists
        if (!out_dir.empty() && !std::filesystem::exists(out_dir))
        {
            std::filesystem::create_directories(out_dir);
        }

        m_status_label->set_text(horizon::i18n().tr("capture.status.recording_screen"));
        std::string filename =
            "recording_" +
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "." +
            container;
        std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;

        // Get monitor dimensions dynamically
        int x = 0, y = 0, w = 1920, h = 1080;
        if (!m_engine.get_output_geometry(output_name, x, y, w, h)) {
            LOG_ERROR << "[CaptureApp] Failed to get monitor geometry, falling back to 1920x1080";
        }

        // Ensure dimensions are even (required for H.264)
        if (w % 2 != 0)
            w--;
        if (h % 2 != 0)
            h--;

        LOG_INFO << "[CaptureApp] Starting video recording to: " << full_path.string() << " (" << w
                 << "x" << h << ") on output " << output_name;

        if (m_recorder->start(output_name, full_path.string(), 0, 0, w, h, 30, true))
        {
            m_is_recording = true;
            NotificationSender::send("Grabación iniciada", "Se está grabando la pantalla completa.",
                                     "media-record");
            m_status_label->set_text(horizon::i18n().tr("capture.status.recording_stop_hint"));
            if (m_record_btn)
            {
                m_record_btn->set_title(horizon::i18n().tr("capture.toolbar.stop"));
                m_record_btn->set_icon_name("media-playback-stop-symbolic");
            }
        }
        else
        {
            m_status_label->set_text(horizon::i18n().tr("capture.status.failed"));
        }
    }

    void CaptureWindow::start_selection_video()
    {
        LOG_INFO << "[CaptureApp] start_selection_video() called";
        m_status_label->set_text(horizon::i18n().tr("capture.status.select_region"));
        m_selection_win = std::make_shared<SelectionWindow>();

        std::thread(
            [win = m_selection_win]()
            {
                win->initialize();
                win->set_visible(true);
                win->run();
            })
            .detach();

        m_selection_win->selection_widget()->when_selected().connect(
            [this](SelectionRect rect)
            {
                int x = rect.x + m_selection_win->screen_x();
                int y = rect.y + m_selection_win->screen_y();
                int w = rect.width;
                int h = rect.height;

                if (w % 2 != 0)
                    w--;
                if (h % 2 != 0)
                    h--;

                m_selection_win->set_visible(false);
                m_selection_win->quit();
                this->application()->post_task([this]() { m_selection_win.reset(); });

                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                m_config->load();
                std::string out_dir =
                    expand_tilde(m_config->get_value("general", "output_directory", "."));
                std::string container = m_config->get_value("video", "container", "mp4");

                if (!out_dir.empty() && !std::filesystem::exists(out_dir))
                {
                    std::filesystem::create_directories(out_dir);
                }

                std::string filename =
                    "recording_selection_" +
                    std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
                    "." + container;
                std::filesystem::path full_path = std::filesystem::path(out_dir) / filename;

                std::string target_output = m_selection_win->w_surface()->current_output_name();
                if (target_output.empty()) {
                    auto outputs = m_engine.get_all_outputs();
                    if (!outputs.empty()) target_output = outputs[0].name;
                }

                LOG_INFO << "[CaptureApp] Starting region recording to: " << full_path.string() << " on output " << target_output;

                if (m_recorder->start(target_output, full_path.string(), x, y, w, h, 30, true))
                {
                    m_is_recording = true;
                    NotificationSender::send("Grabación iniciada",
                                             "Se está grabando la región seleccionada.",
                                             "media-record");
                    m_status_label->set_text(horizon::i18n().tr("capture.status.recording_screen"));
                    if (m_record_btn)
                    {
                        m_record_btn->set_title(horizon::i18n().tr("capture.toolbar.stop"));
                        m_record_btn->set_icon_name("media-playback-stop-symbolic");
                    }
                }
                else
                {
                    m_status_label->set_text(horizon::i18n().tr("capture.status.failed"));
                }
            });

        m_selection_win->selection_widget()->when_cancelled().connect(
            [this](const EventContext &)
            {
                LOG_INFO << "[CaptureApp] Selection cancelled";
                m_selection_win->set_visible(false);
                m_selection_win->quit();
                this->application()->post_task([this]() { m_selection_win.reset(); });
                m_status_label->set_text(horizon::i18n().tr("capture.status.ready"));
            });
    }

    void CaptureWindow::start_window_video()
    {
        m_status_label->set_text(horizon::i18n().tr("capture.status.not_implemented"));
        application()->alert(horizon::i18n().tr("capture.alert.not_implemented_msg"),
                             horizon::i18n().tr("capture.alert.not_implemented_title"));
    }

    void CaptureWindow::stop_video()
    {
        if (!m_is_recording)
            return;
        m_recorder->stop();
        NotificationSender::send("Grabación finalizada", "El video se ha guardado correctamente.",
                                 "media-playback-stop");
        m_is_recording = false;
        m_status_label->set_text(horizon::i18n().tr("capture.status.recording_finished"));
        if (m_record_btn)
        {
            m_record_btn->set_title(horizon::i18n().tr("capture.toolbar.record"));
            m_record_btn->set_icon_name("camera-video-symbolic");
        }
    }

} // namespace horizon::capture
