#pragma once

#include <horizon/Widget.hpp>
#include <horizon/video/VideoDriver.hpp>
#include <horizon/EventsManager.hpp>
#include <memory>
#include <string>

namespace horizon::video {

class VideoView : public Widget {
public:
    VideoView();
    virtual ~VideoView();

    void set_path(const std::string& path);
    const std::string& path() const { return m_path; }

    void play();
    void pause();
    void toggle_play();
    void stop();
    void seek(double seconds);
    
    bool supports_fullscreen() const override { return true; }

    bool is_playing() const;
    double duration() const;
    double position() const;

    void set_aspect_ratio(const std::string& ratio);
    const std::string& aspect_ratio() const { return m_aspect_ratio; }

    std::vector<TrackInfo> audio_tracks() const;
    std::vector<TrackInfo> subtitle_tracks() const;
    void set_audio_track(int id);
    void set_subtitle_track(int id);

    EventsManager<EventContext> when_finished;

protected:
    void draw(GraphicsContext& ctx) override;

private:
    std::unique_ptr<VideoDriver> m_driver;
    std::string m_path;
    std::string m_aspect_ratio{"auto"};

    void ensure_driver();
};

} // namespace horizon::video
