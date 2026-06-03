#include "VolumeIndicator.hpp"
#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Notification.hpp>
#include <horizon/Label.hpp>
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

namespace horizon
{

VolumeIndicator::VolumeIndicator() : ITopPanelWidget()
{
    // Icon
    auto icon = std::make_unique<Icon>();
    m_icon = icon.get();
    m_icon->set_icon_size(20);
    m_icon->set_fixed_size(24);
    m_icon->set_vertical_alignment(VerticalAlignment::Middle);
    m_icon->set_use_theme_colors(true);
    m_icon->set_theme_color_key("window_fg");
    add_child(std::move(icon));

    // Vault and Slider
    auto vault = std::make_unique<Vault>();
    auto vault_content = std::make_unique<Widget>();
    vault_content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
    vault_content->set_margin(10);
    vault_content->set_spacing(10);
    vault_content->set_width(200);

    auto title = std::make_unique<Label>("Volumen");
    title->set_font_weight(FONT_WEIGHT_BOLD);
    vault_content->add_child(std::move(title));

    auto slider = std::make_unique<Slider>();
    m_slider = slider.get();
    m_slider->set_min(0.0f);
    m_slider->set_max(1.0f);
    m_slider->set_value(0.5f);
    vault_content->add_child(std::move(slider));

    vault->set_content(std::move(vault_content));
    this->set_vault(std::move(vault));

    m_slider->when_value_changed.connect([this](EventContext &ctx) {
        float v = m_slider->value();
        // Prevent immediate bounce-back by temporarily ignoring updates in monitor loop?
            // Actually, we can just execute the command.
            std::string cmd = "wpctl set-volume @DEFAULT_AUDIO_SINK@ " + std::to_string(v);
            std::system(cmd.c_str());
            
            // Also unmute if muted and we change volume
            std::system("wpctl set-mute @DEFAULT_AUDIO_SINK@ 0");
    });

    when_application_load.connect([this](EventContext &) {
        m_stop_monitor = false;
        m_monitor_thread = std::thread(&VolumeIndicator::monitor_loop, this);
    });
}

VolumeIndicator::~VolumeIndicator()
{
    m_stop_monitor = true;
    if (m_monitor_thread.joinable()) {
        m_monitor_thread.join();
    }
}

int VolumeIndicator::preferred_width() const
{
    return 24 + (margin() * 2);
}

double VolumeIndicator::get_current_volume()
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("wpctl get-volume @DEFAULT_AUDIO_SINK@", "r"), pclose);
    if (!pipe) {
        return 0.0;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Output is usually "Volume: 0.75" or "Volume: 0.75 [MUTED]"
    size_t pos = result.find("Volume: ");
    if (pos != std::string::npos) {
        try {
            double v = std::stod(result.substr(pos + 8));
            return v;
        } catch (...) {
            return 0.0;
        }
    }
    return 0.0;
}

bool VolumeIndicator::get_current_mute()
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("wpctl get-volume @DEFAULT_AUDIO_SINK@", "r"), pclose);
    if (!pipe) {
        return false;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result.find("[MUTED]") != std::string::npos;
}

void VolumeIndicator::monitor_loop()
{
    LOG_INFO << "VolumeIndicator: Background monitor thread started.";

    while (!m_stop_monitor) {
        double vol = get_current_volume();
        bool muted = get_current_mute();

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(m_state_mutex);
            // Allow small epsilon for float comparison
            if (std::abs(vol - m_current_volume) > 0.01 || muted != m_current_muted) {
                changed = true;
            }
        }

        if (changed && application()) {
            application()->post_task([this, vol, muted]() {
                this->update_ui(vol, muted);
            });
        }

        // Sleep for 1.5 seconds
        for (int i = 0; i < 15 && !m_stop_monitor; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void VolumeIndicator::update_ui(double volume, bool is_muted)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_current_volume = volume;
    m_current_muted = is_muted;

    std::string icon_name = "audio-volume-muted-symbolic";

    if (!is_muted) {
        if (volume > 0.7) {
            icon_name = "audio-volume-high-symbolic";
        } else if (volume > 0.3) {
            icon_name = "audio-volume-medium-symbolic";
        } else if (volume > 0.0) {
            icon_name = "audio-volume-low-symbolic";
        }
    }

    m_icon->set_icon_name(icon_name);
    
    // Update slider only if it's not currently being dragged (simple approach: just update it, slider widget might glitch if being dragged while updated, so we could track it)
    if (!m_slider->is_pressed()) {
        m_slider->set_value(volume);
    }

    auto tip = std::make_unique<Notification>();
    if (is_muted) {
        tip->set_notification(icon_name, "Volumen silenciado");
    } else {
        tip->set_notification(icon_name, "Volumen: " + std::to_string(static_cast<int>(volume * 100)) + "%");
    }
    set_tooltip(std::move(tip));

    invalidate();
}

} // namespace horizon
