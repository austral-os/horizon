#include "ZenitWindow.hpp"
#include <horizon/GraphicsContext.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Statusbar.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Widget.hpp>

namespace zenit
{

    // ... (rest of namespace)

    ZenitWindow::ZenitWindow() : Window("Zenit")
    {
        set_size(1280, 720);

        setup_ui();

        // Connect to standard file events
        when_file_opened.connect([this](const horizon::Window::FileOpenedContext &ev)
                                 { open_file(ev.path); });

        when_file_close.connect(
            [this](const horizon::EventContext &)
            {
                if (m_video_view)
                    m_video_view->stop();
            });
    }

    ZenitWindow::~ZenitWindow() {}

    void ZenitWindow::setup_ui()
    {
        // Main layout container (Vertical)
        auto main_layout = std::make_unique<horizon::Widget>();
        main_layout->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        main_layout->set_position_type(horizon::FILL);

        // 1. Video View (Top area)
        auto video = std::make_unique<horizon::video::VideoView>();
        m_video_view = video.get();
        m_video_view->set_position_type(horizon::FILL);
        main_layout->add_child(std::move(video));

        // 2. Control Bar (Bottom area)
        auto control_bar = std::make_unique<horizon::Statusbar>();
        m_control_bar = control_bar.get();
        m_control_bar->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        m_control_bar->set_fixed_size(60);
        m_control_bar->set_spacing(10);
        m_control_bar->set_margin(0);
        m_control_bar->set_corner_radius({0, 0, 10, 10});

        auto container = std::make_unique<horizon::Widget>();
        container->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        container->set_spacing(10);
        container->set_margin(10);

        // --- Control Bar Widgets ---

        // Rewind
        auto rewind = std::make_unique<horizon::ToolbarButton>("", "media-skip-backward");
        m_rewind_btn = rewind.get();
        m_rewind_btn->set_fixed_size(40);
        m_rewind_btn->when_click.connect(
            [this](horizon::MouseButtonEventContext &)
            {
                m_video_view->seek(m_video_view->position() - 10.0);
                update_controls();
            });
        container->add_child(std::move(rewind));

        // Play/Pause
        auto play_btn = std::make_unique<horizon::ToolbarButton>("", "media-playback-start");
        m_play_pause_btn = play_btn.get();
        m_play_pause_btn->set_fixed_size(40);
        m_play_pause_btn->when_click.connect(
            [this](horizon::MouseButtonEventContext &)
            {
                m_video_view->toggle_play();
                update_controls();
            });
        container->add_child(std::move(play_btn));

        // Forward
        auto forward = std::make_unique<horizon::ToolbarButton>("", "media-skip-forward");
        m_forward_btn = forward.get();
        m_forward_btn->set_fixed_size(40);
        m_forward_btn->when_click.connect(
            [this](horizon::MouseButtonEventContext &)
            {
                m_video_view->seek(m_video_view->position() + 10.0);
                update_controls();
            });
        container->add_child(std::move(forward));

        // Current Time
        auto current_lbl = std::make_unique<horizon::Label>("00:00");
        m_current_time_label = current_lbl.get();
        m_current_time_label->set_margin(5);
        m_current_time_label->set_fixed_size(100);
        container->add_child(std::move(current_lbl));

        // Progress Bar
        auto progress = std::make_unique<horizon::ProgressBar>();
        m_progress_bar = progress.get();
        m_progress_bar->set_position_type(horizon::FILL); // Fills remaining space
        container->add_child(std::move(progress));

        // Total Time
        auto total_lbl = std::make_unique<horizon::Label>("00:00");
        m_total_time_label = total_lbl.get();
        m_total_time_label->set_margin(5);
        m_total_time_label->set_fixed_size(100);
        container->add_child(std::move(total_lbl));

        // Fullscreen (optional but nice to keep)
        auto fs_btn = std::make_unique<horizon::ToolbarButton>("", "view-fullscreen");
        fs_btn->set_fixed_size(40);
        fs_btn->when_click.connect(
            [this](horizon::MouseButtonEventContext &)
            {
                if (application())
                    application()->signal_manager.emit("fullscreen");
            });
        container->add_child(std::move(fs_btn));

        m_control_bar->add_child(std::move(container));

        main_layout->add_child(std::move(control_bar));

        add_child(std::move(main_layout));

        setup_context_menu();
    }

    void ZenitWindow::setup_context_menu()
    {
        auto menu = std::make_unique<horizon::Menu>();

        auto *ar_menu = menu->add_item(horizon::i18n().tr("zenit.aspect_ratio"));
        auto ar_sub_ptr = std::make_unique<horizon::Menu>();
        auto *ar_sub = ar_sub_ptr.get();
        ar_menu->set_submenu(std::move(ar_sub_ptr));

        auto add_ar = [&](const std::string &label, const std::string &ratio)
        {
            auto *item = ar_sub->add_item(label);
            item->when_click.connect([this, ratio](horizon::EventContext &)
                                     { m_video_view->set_aspect_ratio(ratio); });
        };

        add_ar("Auto", "auto");
        add_ar("16:9", "16:9");
        add_ar("4:3", "4:3");
        add_ar("21:9", "21:9");

        m_video_view->set_context_menu(std::move(menu));
    }

    void ZenitWindow::calculate_layout()
    {
        horizon::Window::calculate_layout();
        // Layout is managed by the vertical/horizontal box system now.
    }

    void ZenitWindow::set_application_recursive(horizon::WaylandWindow *app)
    {
        horizon::Window::set_application_recursive(app);
        if (app)
        {
            app->add_timer(500, [this]() { update_controls(); }, true);
        }
    }

    horizon::CornerRadius ZenitWindow::get_window_corners() const
    {
        return {10, 10, 10, 10};
    }

    void ZenitWindow::open_file(const std::string &path)
    {
        if (m_video_view)
        {
            m_video_view->set_path(path);
            m_video_view->play();
            update_controls();
        }
    }

    uint32_t ZenitWindow::file_capabilities() const
    {
        return horizon::FileOpen | horizon::FileClose;
    }

    std::string ZenitWindow::current_file_path() const
    {
        return m_video_view ? m_video_view->path() : "";
    }

    void ZenitWindow::update_controls()
    {
        if (m_video_view->is_playing())
        {
            m_play_pause_btn->set_icon_name("media-playback-pause");
        }
        else
        {
            m_play_pause_btn->set_icon_name("media-playback-start");
        }

        double pos = m_video_view->position();
        double dur = m_video_view->duration();

        auto format_time = [](double seconds) -> std::string
        {
            int s = (int)seconds;
            int m = s / 60;
            int h = m / 60;
            s %= 60;
            m %= 60;
            char buf[32];
            if (h > 0)
                sprintf(buf, "%02d:%02d:%02d", h, m, s);
            else
                sprintf(buf, "%02d:%02d", m, s);
            return buf;
        };

        if (m_current_time_label)
            m_current_time_label->set_text(format_time(pos));
        if (m_total_time_label)
            m_total_time_label->set_text(format_time(dur));

        if (dur > 0)
        {
            m_progress_bar->set_progress((float)(pos / dur));
        }
    }

} // namespace zenit
