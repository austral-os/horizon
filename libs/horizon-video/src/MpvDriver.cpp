#include <horizon/video/VideoDriver.hpp>
#include <mpv/client.h>
#include <mpv/render.h>
#include <horizon/Logger.hpp>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <atomic>

namespace horizon::video {
namespace internal {

class MpvDriver : public VideoDriver {
public:
    MpvDriver() {
        m_mpv = mpv_create();
        if (!m_mpv) throw std::runtime_error("Could not create mpv instance");

        mpv_set_option_string(m_mpv, "vo", "libmpv");
        mpv_set_option_string(m_mpv, "hwdec", "auto");
        mpv_set_option_string(m_mpv, "keep-open", "yes");

        if (mpv_initialize(m_mpv) < 0) {
            mpv_terminate_destroy(m_mpv);
            throw std::runtime_error("Could not initialize mpv");
        }

        // Setup render context for software rendering
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_SW},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        if (mpv_render_context_create(&m_render_ctx, m_mpv, params) < 0) {
             LOG_ERROR << "[MpvDriver] Could not create render context";
        }

        // Start event thread
        m_running = true;
        m_event_thread = std::thread([this]() {
            while (m_running) {
                mpv_event* event = mpv_wait_event(m_mpv, 0.1); // Wait up to 100ms
                if (event->event_id == MPV_EVENT_NONE) continue;
                if (event->event_id == MPV_EVENT_SHUTDOWN) {
                    LOG_INFO << "[MpvDriver] Shutdown event received";
                    break;
                }

                if (event->event_id == MPV_EVENT_END_FILE) {
                    LOG_INFO << "[MpvDriver] EOF detected in event thread";
                    if (on_finished) on_finished();
                } else {
                    LOG_INFO << "[MpvDriver] Event: " << mpv_event_name(event->event_id);
                }
            }
        });
    }

    virtual ~MpvDriver() {
        m_running = false;
        if (m_event_thread.joinable()) m_event_thread.join();
        
        if (m_render_ctx) mpv_render_context_free(m_render_ctx);
        if (m_mpv) mpv_terminate_destroy(m_mpv);
    }

    bool load(const std::string& path) override {
        const char* cmd[] = {"loadfile", path.c_str(), nullptr};
        return mpv_command(m_mpv, cmd) >= 0;
    }

    void play() override {
        // If we reached EOF, seek to beginning first
        int eof = 0;
        mpv_get_property(m_mpv, "eof-reached", MPV_FORMAT_FLAG, &eof);
        if (eof) {
            const char* cmd[] = {"seek", "0", "absolute", nullptr};
            mpv_command(m_mpv, cmd);
        }

        int pause = 0;
        mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &pause);
    }

    void pause() override {
        int pause = 1;
        mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &pause);
    }

    void seek(double seconds) override {
        const char* cmd[] = {"seek", std::to_string(seconds).c_str(), "absolute", nullptr};
        mpv_command(m_mpv, cmd);
    }

    double duration() const override {
        double d = 0;
        mpv_get_property(m_mpv, "duration", MPV_FORMAT_DOUBLE, &d);
        return d;
    }

    double position() const override {
        double p = 0;
        mpv_get_property(m_mpv, "time-pos", MPV_FORMAT_DOUBLE, &p);
        return p;
    }

    bool is_playing() const override {
        int pause = 1;
        mpv_get_property(m_mpv, "pause", MPV_FORMAT_FLAG, &pause);
        
        int eof = 0;
        mpv_get_property(m_mpv, "eof-reached", MPV_FORMAT_FLAG, &eof);
        
        return !pause && !eof;
    }

    void draw(GraphicsContext& ctx, int x, int y, int w, int h) override {
        if (!m_render_ctx || w <= 0 || h <= 0) return;

        int stride = w * 4;
        std::vector<uint8_t> buffer(stride * h);

        int size[] = {w, h};
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_SW_SIZE, size},
            {MPV_RENDER_PARAM_SW_STRIDE, &stride},
            {MPV_RENDER_PARAM_SW_POINTER, buffer.data()},
            {MPV_RENDER_PARAM_SW_FORMAT, (void*)"bgra"},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        mpv_render_context_render(m_render_ctx, params);

        ctx.drawPixels(buffer.data(), w, h, x, y, w, h, 4);
        
        if (on_position_changed) on_position_changed(position());
    }

    void set_aspect_ratio(const std::string& ratio) override {
        mpv_set_property_string(m_mpv, "video-aspect-override", ratio.c_str());
    }

    std::vector<TrackInfo> get_audio_tracks() const override {
        return get_tracks("audio");
    }

    std::vector<TrackInfo> get_subtitle_tracks() const override {
        return get_tracks("sub");
    }

    void set_audio_track(int id) override {
        mpv_set_property_string(m_mpv, "aid", std::to_string(id).c_str());
    }

    void set_subtitle_track(int id) override {
        mpv_set_property_string(m_mpv, "sid", std::to_string(id).c_str());
    }

private:
    mpv_handle* m_mpv = nullptr;
    mpv_render_context* m_render_ctx = nullptr;
    std::thread m_event_thread;
    std::atomic<bool> m_running{false};

    std::vector<TrackInfo> get_tracks(const std::string& type) const {
        std::vector<TrackInfo> tracks;
        return tracks;
    }
};

std::unique_ptr<VideoDriver> create_mpv_driver() {
    try {
        return std::make_unique<MpvDriver>();
    } catch (...) {
        return nullptr;
    }
}

} // namespace internal
} // namespace horizon::video
