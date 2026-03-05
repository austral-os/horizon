#include "horizon/Image.hpp"
#include "horizon/Application.hpp"
#include <algorithm>

namespace horizon
{
    Image::Image() : Widget() {}

    Image::~Image() = default;

    void Image::set_path(const std::string &path)
    {
        if (m_path == path)
            return;

        m_path = path;
        load_driver();
        invalidate();
    }

    void Image::set_mode(ImageMode mode)
    {
        if (m_mode == mode)
            return;

        m_mode = mode;
        invalidate();
    }

    void Image::set_application_recursive(Application *app)
    {
        Widget::set_application_recursive(app);
        if (app && !m_driver && !m_path.empty())
        {
            load_driver();
        }
    }

    void Image::load_driver()
    {
        m_driver.reset();
        if (m_path.empty() || !application())
            return;

        // Image doesn't know about Stb or Svg anymore.
        // It asks the current context to create a driver for it.
        // This makes it compatible with future rendering backends.
        m_driver = application()->get_graphics_context().createImageDriver(m_path);
    }

    void Image::draw(GraphicsContext &ctx)
    {
        if (!m_driver)
            return;

        int img_w = m_driver->width();
        int img_h = m_driver->height();

        if (img_w <= 0 || img_h <= 0)
            return;

        int draw_x = m_start_draw_x;
        int draw_y = m_start_draw_y;
        int draw_w = m_available_draw_width;
        int draw_h = m_available_draw_height;

        ctx.save();
        ctx.clip(draw_x, draw_y, draw_w, draw_h);

        switch (m_mode)
        {
        case ImageMode::Normal:
            // Center the image at its original size
            {
                int x = draw_x + (draw_w - img_w) / 2;
                int y = draw_y + (draw_h - img_h) / 2;
                m_driver->draw(ctx, x, y, img_w, img_h);
            }
            break;

        case ImageMode::Stretch:
            // Fill the entire area
            m_driver->draw(ctx, draw_x, draw_y, draw_w, draw_h);
            break;

        case ImageMode::Fit:
            // Maintain aspect ratio
            {
                double aspect_ratio = static_cast<double>(img_w) / img_h;
                int target_w = draw_w;
                int target_h = static_cast<int>(draw_w / aspect_ratio);

                if (target_h > draw_h)
                {
                    target_h = draw_h;
                    target_w = static_cast<int>(draw_h * aspect_ratio);
                }

                int x = draw_x + (draw_w - target_w) / 2;
                int y = draw_y + (draw_h - target_h) / 2;
                m_driver->draw(ctx, x, y, target_w, target_h);
            }
            break;

        case ImageMode::Repeat:
            // Tiling
            {
                for (int y = draw_y; y < draw_y + draw_h; y += img_h)
                {
                    for (int x = draw_x; x < draw_x + draw_w; x += img_w)
                    {
                        m_driver->draw(ctx, x, y, img_w, img_h);
                    }
                }
            }
            break;
        }

        ctx.restore();
    }
} // namespace horizon
