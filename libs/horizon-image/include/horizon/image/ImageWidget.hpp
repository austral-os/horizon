#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ImageDriver.hpp>
#include <string>
#include <memory>

namespace horizon {
namespace image {

class ImageWidget : public Widget {
public:
    ImageWidget();
    virtual ~ImageWidget();

    void set_path(const std::string& path);
    const std::string& path() const { return m_path; }

    void set_zoom(float zoom);
    float zoom() const { return m_zoom; }

    void set_rotation(float rotation); // In degrees
    float rotation() const { return m_rotation; }

    void zoom_in();
    void zoom_out();
    void zoom_fit(int container_w, int container_h);
    void original_size();

    void rotate_cw();  // +90 deg
    void rotate_ccw(); // -90 deg

    int image_width() const;
    int image_height() const;
    
    bool supports_fullscreen() const override { return true; }

    void set_application_recursive(WaylandWindow* app) override;

protected:
    void draw(GraphicsContext& ctx) override;

private:
    std::string m_path;
    std::unique_ptr<ImageDriver> m_driver;
    float m_zoom{1.0f};
    float m_rotation{0.0f};
    
    void load_driver();
    void update_size();
    void setup_context_menu();
};

} // namespace image
} // namespace horizon
