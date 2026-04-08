#pragma once

#include "horizon/Widget.hpp"
#include "horizon/SignalManager.hpp"
#include <cairo.h>
#include <thread>
#include <mutex>

typedef struct _GMainContext GMainContext;
typedef struct _GMainLoop GMainLoop;
typedef struct _WebKitWebView WebKitWebView;
typedef struct _GParamSpec GParamSpec;
struct wpe_view_backend;
struct wpe_view_backend_exportable_fdo;
struct wpe_fdo_shm_exported_buffer;

namespace horizon {
namespace web {

class WebWidget : public horizon::Widget {
public:
    WebWidget();
    virtual ~WebWidget();

    /**
     * @brief Explicitly shuts down the worker thread and cleans up global WPE state.
     */
    static void shutdown();

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
    std::mutex m_surface_mutex;
    cairo_surface_t* m_cairo_surface = nullptr;
    
    bool m_initialized = false;
    
    // Threading (Shared across all instances)
    static void ensure_worker_thread();
    static std::thread s_worker_thread;
    static GMainContext* s_worker_context;
    static GMainLoop* s_worker_loop;
    static bool s_running;
    static std::mutex s_worker_mutex;
    static void worker_thread_func();
    
    std::string m_cached_title;
    std::string m_cached_url;
    mutable std::mutex m_metadata_mutex;
};

} // namespace web
} // namespace horizon
