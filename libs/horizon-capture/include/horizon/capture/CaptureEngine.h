#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <wayland-client.h>
#include <protocols/wlr-screencopy-unstable-v1-client-protocol.h>

namespace horizon::capture {

struct CaptureBuffer {
    void* data;
    size_t size;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
};

class CaptureEngine {
public:
    CaptureEngine();
    ~CaptureEngine();

    bool init();
    
    /**
     * @brief Capture a screenshot of the entire output and save it to a file.
     * @param output_name Name of the output (e.g., "eDP-1"). If empty, captures the first output.
     * @param file_path Path to save the image (PNG).
     * @return true if successful.
     */
    bool capture_screenshot(const std::string& output_name, const std::string& file_path);

    /**
     * @brief Get the dimensions of an output.
     * @param output_name Name of the output. If empty, uses the first one.
     * @param w Output width.
     * @param h Output height.
     * @return true if successful.
     */
    bool get_output_dimensions(const std::string& output_name, int& w, int& h);
    
    /**
     * @brief Capture a specific region of an output.
     * @param output_name Name of the output.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param width Width of the region.
     * @param height Height of the region.
     * @param file_path Path to save the image.
     * @return true if successful.
     */
    bool capture_region(const std::string& output_name, int x, int y, int width, int height, const std::string& file_path);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace horizon::capture
