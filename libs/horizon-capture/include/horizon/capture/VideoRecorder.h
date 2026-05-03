#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

namespace horizon::capture {

class VideoRecorder {
public:
    VideoRecorder();
    ~VideoRecorder();

    /**
     * @brief Start recording a screen region to a video file.
     * @param output_file Path to the output video file (e.g. .mp4).
     * @param x Global X coordinate of the region.
     * @param y Global Y coordinate of the region.
     * @param width Width of the region.
     * @param height Height of the region.
     * @param fps Target frames per second.
     * @return true if recording started successfully.
     */
    bool start(const std::string& output_file, int x, int y, int width, int height, int fps = 30);

    /**
     * @brief Stop the current recording.
     */
    void stop();

    /**
     * @brief Check if a recording is currently in progress.
     */
    bool is_recording() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace horizon::capture
