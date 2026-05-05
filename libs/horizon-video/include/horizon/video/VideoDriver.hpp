#pragma once
#include <horizon/GraphicsContext.hpp>
#include <string>
#include <functional>
#include <vector>

namespace horizon::video {

struct TrackInfo {
    int id;
    std::string language;
    std::string title;
    bool selected;
};

class VideoDriver {
public:
    virtual ~VideoDriver() = default;

    virtual bool load(const std::string& path) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void seek(double seconds) = 0;
    
    virtual double duration() const = 0;
    virtual double position() const = 0;
    virtual bool is_playing() const = 0;

    virtual void draw(GraphicsContext& ctx, int x, int y, int w, int h) = 0;

    virtual void set_aspect_ratio(const std::string& ratio) = 0;
    
    virtual std::vector<TrackInfo> get_audio_tracks() const = 0;
    virtual std::vector<TrackInfo> get_subtitle_tracks() const = 0;
    
    virtual void set_audio_track(int id) = 0;
    virtual void set_subtitle_track(int id) = 0;

    // Callbacks
    std::function<void(double)> on_position_changed;
    std::function<void()> on_finished;
};

} // namespace horizon::video
