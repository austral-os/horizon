#pragma once

#include <horizon/image/ImageWidget.hpp>
#include <string>

namespace horizon {
namespace image {

class CroppableImageWidget : public ImageWidget {
public:
    CroppableImageWidget();
    ~CroppableImageWidget() override;

    void toggle_crop_mode();
    void apply_crop();
    void save_image();
    void cancel_crop();
    void undo_crop();
    void redo_crop();

    bool supports_undo() const override { return true; }
    // By convention, supporting undo often implies supporting redo, but let's make sure.

protected:
    void draw(GraphicsContext& ctx) override;

private:
    bool m_is_cropping{false};
    std::vector<std::string> m_history;
    std::vector<std::string> m_redo_history;

    
    // Crop rect in image coordinates (0,0 is top-left of image)
    float m_crop_x{0};
    float m_crop_y{0};
    float m_crop_w{0};
    float m_crop_h{0};

    // Interaction states
    int m_dragging_handle{-1}; // -1: none, 0: top-left, 1: top-right, 2: bottom-right, 3: bottom-left, 4: center
    float m_drag_start_x{0};
    float m_drag_start_y{0};
    float m_crop_start_x{0};
    float m_crop_start_y{0};
    float m_crop_start_w{0};
    float m_crop_start_h{0};

    std::string m_original_path;
    std::string m_current_temp_path;

    void screen_to_image(float sx, float sy, float& ix, float& iy);
    void reset_crop_rect();
};

} // namespace image
} // namespace horizon
