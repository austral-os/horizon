#pragma once

#include "horizon/Widget.hpp"
#include "horizon/SignalManager.hpp"
#include <cairo.h>
#include <memory>
#include <string>

// Forward declarations for WPE/WebKit
typedef struct _WebKitWebView WebKitWebView;
struct wpe_view_backend;
struct wpe_view_backend_exportable_fdo;
struct wpe_fdo_shm_exported_buffer;
typedef struct _GParamSpec GParamSpec;


namespace horizon {
namespace web {

class WebWidget : public Widget {
public:
    WebWidget();
    virtual ~WebWidget();

    void load_url(const std::string& url);
    void reload();
    void stop_loading();
    void go_back();
    void go_forward();

    bool can_go_back() const;
    bool can_go_forward() const;
    
    std::string get_title() const;
    std::string get_url() const;

    // Signals
    EventsManager<std::string> when_title_changed;
    EventsManager<std::string> when_url_changed;
    EventsManager<bool> when_loading_changed;
    EventsManager<double> when_progress_changed;


protected:
    void draw(GraphicsContext& ctx) override;
    void calculate_layout() override;

private:
    void init_wpe();
    static void on_frame_exported(void* data, struct wpe_fdo_shm_exported_buffer* buffer);
    
    // WebKit Signal Handlers (Static for C-style callbacks)
    static void on_title_notify(WebKitWebView* web_view, GParamSpec* pspec, WebWidget* self);
    static void on_uri_notify(WebKitWebView* web_view, GParamSpec* pspec, WebWidget* self);
    static void on_load_changed(WebKitWebView* web_view, int load_event, WebWidget* self);
    static void on_progress_notify(WebKitWebView* web_view, GParamSpec* pspec, WebWidget* self);
    static void on_mouse_target_changed(WebKitWebView* web_view, void* hit_test_result, uint32_t modifiers, WebWidget* self);

    WebKitWebView* m_web_view = nullptr;
    struct wpe_view_backend* m_backend = nullptr;
    struct wpe_view_backend_exportable_fdo* m_exportable = nullptr;
    
    // For SHM rendering (easier to bridge to Cairo)
    cairo_surface_t* m_cairo_surface = nullptr;
    
    bool m_initialized = false;
};

} // namespace web
} // namespace horizon
