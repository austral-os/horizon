#include <horizon/video/VideoDriver.hpp>
#include <mpv/client.h>
#include <mpv/render.h>
#include <horizon/Logger.hpp>
#include <stdexcept>
#include <mutex>

namespace horizon::video {
namespace internal {

class MpvDriver : public VideoDriver {
public:
    MpvDriver() {
        m_mpv = mpv_create();
        if (!m_mpv) throw std::runtime_error("Could not create mpv instance");

        mpv_set_option_string(m_mpv, "vo", "libmpv");
        mpv_set_option_string(m_mpv, "hwdec", "auto");

        if (mpv_initialize(m_mpv) < 0) {
            mpv_terminate_destroy(m_mpv);
            throw std::runtime_error("Could not initialize mpv");
        }

        // Setup render context for software rendering (simplest for Cairo integration)
        // In the future, we could use OpenGL for better performance
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_SW},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        if (mpv_render_context_create(&m_render_ctx, m_mpv, params) < 0) {
             LOG_ERROR << "[MpvDriver] Could not create render context";
        }
    }

    virtual ~MpvDriver() {
        if (m_render_ctx) mpv_render_context_free(m_render_ctx);
        if (m_mpv) mpv_terminate_destroy(m_mpv);
    }

    bool load(const std::string& path) override {
        const char* cmd[] = {"loadfile", path.c_str(), nullptr};
        return mpv_command(m_mpv, cmd) >= 0;
    }

    void play() override {
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
        return !pause;
    }

    void draw(GraphicsContext& ctx, int x, int y, int w, int h) override {
        if (!m_render_ctx || w <= 0 || h <= 0) return;

        // Software rendering implementation
        // We render to a buffer and then use ctx.drawPixels
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
        
        // Notify position change (this should be done via mpv events, but for now simple)
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

    std::vector<TrackInfo> get_tracks(const std::string& type) const {
        std::vector<TrackInfo> tracks;
        // In a real implementation, we would parse the 'track-list' property
        // For now, this is a skeleton
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
