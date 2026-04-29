#include "horizon/download/DownloadItemWidget.hpp"
#include "horizon/Spacer.hpp"
#include <iomanip>
#include <sstream>
#include "horizon/I18n.hpp"

namespace horizon {
namespace download {

DownloadItemWidget::DownloadItemWidget(std::shared_ptr<DownloadTask> task)
    : m_task(task) {
    
    set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);
    set_margin(5);
    set_height(90);
    set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
    set_border_radius(8);
    set_border_width(1);
    set_border_color(Color(0.85f, 0.85f, 0.85f, 1.0f));

    // 1. Icon
    auto icon_container = std::make_unique<horizon::Widget>();
    icon_container->set_size(60, 90);
    icon_container->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
    
    auto icon = std::make_unique<horizon::Icon>();
    icon->set_icon_name("folder-download-symbolic");
    icon->set_icon_size(32);
    icon->set_margin(14); // Center it vertically roughly
    icon_container->add_child(std::move(icon));
    add_child(std::move(icon_container));

    // 2. Middle column (Name + Progress + Status)
    auto middle = std::make_unique<horizon::Widget>();
    middle->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
    middle->set_margin(10);
    middle->set_spacing(5);
    
    auto name_lbl = std::make_unique<horizon::Label>(m_task->filename());
    m_name_label = name_lbl.get();
    m_name_label->set_font_size(13);
    m_name_label->set_font_weight(FONT_WEIGHT_BOLD);
    m_name_label->set_alignment(horizon::TextAlignment::Left);
    middle->add_child(std::move(name_lbl));

    auto pb = std::make_unique<horizon::ProgressBar>();
    m_progress_bar = pb.get();
    m_progress_bar->set_height(8);
    middle->add_child(std::move(pb));

    auto status_lbl = std::make_unique<horizon::Label>(i18n().tr("download.status.starting"));
    m_status_label = status_lbl.get();
    m_status_label->set_font_size(11);
    m_status_label->set_text_color(Color(0.4f, 0.4f, 0.4f, 1.0f));
    m_status_label->set_alignment(horizon::TextAlignment::Left);
    middle->add_child(std::move(status_lbl));

    add_child(std::move(middle));

    // 3. Actions
    auto actions = std::make_unique<horizon::Widget>();
    actions->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_HORIZONTAL);
    actions->set_margin(10);
    actions->set_spacing(8);
    actions->set_width(100);

    auto action_btn = std::make_unique<horizon::Button<horizon::SolidObject>>();
    m_action_button = action_btn.get();
    m_action_button->set_size(34, 34);
    
    auto a_icon = std::make_unique<horizon::Icon>();
    a_icon->set_icon_name("media-playback-pause-symbolic");
    m_action_icon = a_icon.get();
    m_action_icon->set_icon_size(16);
    m_action_button->add_child(std::move(a_icon));

    m_action_button->when_click.connect([this](horizon::MouseButtonEventContext&) {
        if (m_task->state() == DownloadState::DOWNLOADING) m_task->pause();
        else m_task->resume();
    });
    actions->add_child(std::move(action_btn));

    auto cancel_btn = std::make_unique<horizon::Button<horizon::SolidObject>>();
    m_cancel_button = cancel_btn.get();
    m_cancel_button->set_size(34, 34);
    
    auto c_icon = std::make_unique<horizon::Icon>();
    c_icon->set_icon_name("process-stop-symbolic");
    c_icon->set_icon_size(16);
    m_cancel_button->add_child(std::move(c_icon));

    m_cancel_button->when_click.connect([this](horizon::MouseButtonEventContext&) {
        m_task->cancel();
    });
    actions->add_child(std::move(cancel_btn));

    add_child(std::move(actions));

    // Connect task signals
    m_progress_conn = m_task->when_progress_changed.connect([this](DownloadProgress&) {
        if (application()) {
            application()->post_task([this]() {
                if (application() && application()->is_widget_alive(this)) {
                    this->update_ui();
                }
            });
        }
    });

    m_state_conn = m_task->when_state_changed.connect([this](DownloadState) {
        if (application()) {
            application()->post_task([this]() {
                if (application() && application()->is_widget_alive(this)) {
                    this->update_ui();
                }
            });
        }
    });

    update_ui();
}

DownloadItemWidget::~DownloadItemWidget() {
    if (m_task) {
        m_task->when_progress_changed.disconnect(m_progress_conn);
        m_task->when_state_changed.disconnect(m_state_conn);
    }
}

void DownloadItemWidget::calculate_layout() {
    // Distribute width: middle takes the rest
    int icon_w = 60;
    int actions_w = 100;
    int middle_w = width() - icon_w - actions_w - 20; // 20 for gaps
    
    if (m_children.size() >= 3) {
        m_children[0]->set_width(icon_w);
        m_children[1]->set_width(middle_w);
        m_children[2]->set_width(actions_w);
    }

    horizon::Widget::calculate_layout();
}

void DownloadItemWidget::update_ui() {
    auto p = m_task->progress();
    m_progress_bar->set_progress((float)p.progress);

    std::stringstream ss;
    if (m_task->state() == DownloadState::DOWNLOADING) {
        ss << format_size(p.downloaded_bytes) << " / " << format_size(p.total_bytes);
        ss << " (" << format_size((size_t)p.speed) << "/s)";
        m_action_icon->set_icon_name("media-playback-pause-symbolic");
        m_action_button->set_visible(true);
    } else if (m_task->state() == DownloadState::PAUSED) {
        ss << i18n().tr("download.status.paused") << " - " << format_size(p.downloaded_bytes);
        m_action_icon->set_icon_name("media-playback-start-symbolic");
        m_action_button->set_visible(true);
    } else if (m_task->state() == DownloadState::COMPLETED) {
        ss << i18n().tr("download.status.completed") << " (" << format_size(p.total_bytes) << ")";
        m_action_button->set_visible(false);
    } else if (m_task->state() == DownloadState::FAILED) {
        ss << i18n().tr("download.status.failed") << ": " << m_task->error_message();
        m_action_icon->set_icon_name("view-refresh-symbolic");
        m_action_button->set_visible(true);
    }

    m_status_label->set_text(ss.str());
}

std::string DownloadItemWidget::format_size(size_t bytes) {
    if (bytes == 0) return "0 B";
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double d_bytes = (double)bytes;
    while (d_bytes >= 1024 && i < 4) {
        d_bytes /= 1024;
        i++;
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << d_bytes << " " << units[i];
    return ss.str();
}

} // namespace download
} // namespace horizon
