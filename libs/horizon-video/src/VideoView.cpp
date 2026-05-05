#include <horizon/video/VideoView.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Logger.hpp>

namespace horizon::video {

VideoView::VideoView() {
    set_focusable(true);
}

VideoView::~VideoView() {
}

void VideoView::set_path(const std::string& path) {
    if (m_path == path) return;
    m_path = path;
    
    ensure_driver();
    if (m_driver) {
        m_driver->load(path);
        invalidate();
    }
}

void VideoView::play() {
    ensure_driver();
    if (m_driver) {
        m_driver->play();
    }
}

void VideoView::pause() {
    if (m_driver) m_driver->pause();
}

void VideoView::toggle_play() {
    if (m_driver) {
        if (m_driver->is_playing()) m_driver->pause();
        else m_driver->play();
    }
}

void VideoView::stop() {
    if (m_driver) m_driver->pause(); // Simple stop for now
}

void VideoView::seek(double seconds) {
    if (m_driver) m_driver->seek(seconds);
}

bool VideoView::is_playing() const {
    return m_driver ? m_driver->is_playing() : false;
}

double VideoView::duration() const {
    return m_driver ? m_driver->duration() : 0.0;
}

double VideoView::position() const {
    return m_driver ? m_driver->position() : 0.0;
}

void VideoView::set_aspect_ratio(const std::string& ratio) {
    m_aspect_ratio = ratio;
    if (m_driver) m_driver->set_aspect_ratio(ratio);
    invalidate();
}

std::vector<TrackInfo> VideoView::audio_tracks() const {
    return m_driver ? m_driver->get_audio_tracks() : std::vector<TrackInfo>{};
}

std::vector<TrackInfo> VideoView::subtitle_tracks() const {
    return m_driver ? m_driver->get_subtitle_tracks() : std::vector<TrackInfo>{};
}

void VideoView::set_audio_track(int id) {
    if (m_driver) m_driver->set_audio_track(id);
}

void VideoView::set_subtitle_track(int id) {
    if (m_driver) m_driver->set_subtitle_track(id);
}

void VideoView::draw(GraphicsContext& ctx) {
    // Always draw black background first
    ctx.setColor(0, 0, 0, 1);
    ctx.fillRect(x(), y(), width(), height());

    if (m_driver) {
        m_driver->draw(ctx, x(), y(), width(), height());
    }
}

// Forward declaration of the driver implementation
// In a real implementation, we would have a factory or a specific inclusion
namespace internal {
    std::unique_ptr<VideoDriver> create_mpv_driver();
}

void VideoView::ensure_driver() {
    if (!m_driver) {
        m_driver = internal::create_mpv_driver();
        if (m_driver) {
            m_driver->on_position_changed = [this](double) {
                invalidate(); // Request redraw on each frame/position change
            };
            m_driver->on_finished = [this]() {
                EventContext ctx;
                ctx.sender = this;
                when_finished.run(ctx);
            };
        }
    }
}

} // namespace horizon::video
