#pragma once

#include "horizon/Widget.hpp"
#include <cairo.h>
#include <glib.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>


typedef struct _GMainContext GMainContext;
typedef struct _GMainLoop GMainLoop;
typedef struct _WebKitWebView WebKitWebView;
typedef struct _WebKitUserContentManager WebKitUserContentManager;
typedef struct _WebKitJavascriptResult WebKitJavascriptResult;
typedef struct _GParamSpec GParamSpec;
typedef struct _WebKitPolicyDecision WebKitPolicyDecision;
struct wpe_view_backend;
struct wpe_view_backend_exportable_fdo;
struct wpe_fdo_shm_exported_buffer;

namespace horizon {
class WaylandWindow;
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
    EventsManager<bool> when_fullscreen_changed;


protected:
    void draw(GraphicsContext& ctx) override;
    void calculate_layout() override;

private:
    void init_wpe();
    static void on_frame_exported(void* data, struct wpe_fdo_shm_exported_buffer* buffer);
    
    // WebKit Signal Handlers (Static for C-style callbacks)
    static void on_title_notify(void* web_view, void* pspec, void* self);
    static void on_uri_notify(void* web_view, void* pspec, void* self);
    static void on_load_changed(void* web_view, int load_event, void* self);
    static void on_progress_notify(void* web_view, void* pspec, void* self);
    static void on_mouse_target_changed(void* web_view, void* hit_test_result, uint32_t modifiers, void* self);
    static int on_enter_fullscreen(void* web_view, void* self);
    static int on_leave_fullscreen(void* web_view, void* self);
    static int on_permission_request(void* web_view, void* request, void* self);
    static int on_decide_policy(void* web_view, void* decision, int type, void* self);

    void update_scrollbars();
    void handle_ui_scroll(int x, int y);

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

    // Scroll State
    double m_content_width = 0;
    double m_content_height = 0;
    double m_scroll_x = 0;
    double m_scroll_y = 0;
    
    bool m_show_v_scroll = false;
    bool m_show_h_scroll = false;
    
    bool m_is_dragging_v = false;
    bool m_is_dragging_h = false;
    int m_drag_start_pos = 0;
    double m_drag_start_scroll = 0;

    int m_v_track_x=0, m_v_track_y=0, m_v_track_w=0, m_v_track_h=0;
    int m_h_track_x=0, m_h_track_y=0, m_h_track_w=0, m_h_track_h=0;
    
    int m_v_thumb_y=0, m_v_thumb_h=0;
    int m_h_thumb_x=0, m_h_thumb_w=0;
    
    int m_last_dispatched_width = 0;
    int m_last_dispatched_height = 0;

    // Production-grade stability members
    std::atomic<bool> m_scroll_dirty{false};
    double m_target_scroll_x = 0, m_target_scroll_y = 0;
    double m_target_content_w = 0, m_target_content_h = 0;
    
    std::chrono::steady_clock::time_point m_last_v_show_time;
    std::chrono::steady_clock::time_point m_last_h_show_time;

    bool m_is_fullscreen = false;
    bool m_pending_fullscreen_ack = false;
    bool m_waiting_for_native_frame = false;
    unsigned int m_fullscreen_ack_timer = 0;
    std::chrono::steady_clock::time_point m_last_fullscreen_time;

    mutable std::mutex m_scroll_mutex;
    static constexpr int SCROLLBAR_SIZE = 12;

};

} // namespace web
} // namespace horizon
