#include "SplashApplication.hpp"
#include "SplashWindow.hpp"
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <horizon/Logger.hpp>
#include <horizon/I18n.hpp>
#include <nlohmann/json.hpp>

namespace horizon
{
    SplashApplication::SplashApplication()
        : Application("org.horizon.splash", 800, 600, true, true)
    {
        m_start_time = std::chrono::steady_clock::now();

        i18n().load_app_locales("horizon-splash");

        auto &about = about_manager();
        about.set_app_title(i18n().tr("horizon_splash.title"));
        about.set_app_description(i18n().tr("horizon_splash.description"));
        about.set_app_version("1.0.0");
        about.set_app_icon("emblem-austral");

        // Create a LayerWindow for the splash screen
        m_window = create_layer_window("org.horizon.splash", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY);
        
        m_window->set_name("Horizon Splash");

        // Full screen anchor
        m_window->set_anchor(ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

        // Do not steal keyboard focus unnecessarily, we just want to be an overlay
        m_window->set_keyboard_interactivity(ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);

        // Set exclusive zone to -1 to ignore panels and be truly fullscreen
        m_window->set_exclusive_zone(-1);

        // No blur, we want it completely opaque
        m_window->set_blur(false);

        // Create the splash widget
        auto splash_widget = std::make_unique<SplashWindow>();
        m_splash_widget = splash_widget.get();

        // Set as root
        m_window->set_root(std::move(splash_widget));
        m_window->set_visible(true);

        // Setup IPC server
        m_ipc_server = std::make_unique<IpcServer>("/tmp/horizon_splash.sock",
            [this](const std::string &msg) -> std::string {
                return this->handle_ipc_message(msg);
            });
        m_ipc_server->start();
    }

    SplashApplication::~SplashApplication()
    {
        if (m_ipc_server) {
            m_ipc_server->stop();
        }
    }

    std::string SplashApplication::handle_ipc_message(const std::string &msg)
    {
        try {
            nlohmann::json j = nlohmann::json::parse(msg);
            
            if (j.contains("close") && j["close"].get<bool>()) {
                LOG_INFO << "[horizon-splash] Received close signal.";
                
                post_task([this]() {
                    if (m_window) {
                        m_window->quit();
                    }
                });
                return "{\"status\":\"ok\"}";
            }

            std::string text = "";
            int progress = 0;

            if (j.contains("text_key")) {
                std::string key = j["text_key"].get<std::string>();
                horizon::Params params;
                if (j.contains("text_params") && j["text_params"].is_object()) {
                    for (auto &[k, v] : j["text_params"].items()) {
                        params[k] = v.get<std::string>();
                    }
                }
                text = i18n().tr(key, params);
            } else if (j.contains("text")) {
                text = j["text"].get<std::string>();
            }
            if (j.contains("progress")) {
                progress = j["progress"].get<int>();
            }

            post_task([this, text, progress]() {
                if (m_splash_widget) {
                    m_splash_widget->update_status(text, progress);
                }
            });

            return "{\"status\":\"ok\"}";
        } catch (const std::exception &e) {
            LOG_ERROR << "[horizon-splash] Error parsing IPC message: " << e.what();
            return "{\"status\":\"error\"}";
        }
    }
} // namespace horizon
