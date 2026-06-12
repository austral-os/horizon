#pragma once
#include <horizon/ImageDriver.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{
    /**
     * @brief Image display modes.
     */
    enum class ImageMode
    {
        Normal,  ///< Draw at original size, centered.
        Stretch, ///< Stretch to fill the entire widget area.
        Fit,     ///< Scale to fill while maintaining aspect ratio.
        Repeat,  ///< Tile the image to fill the entire area.
        Crop     ///< Scale to fill while maintaining aspect ratio, cropping the excess.
    };

    class WaylandWindow;

    /**
     * @brief A widget that displays an image from a file path.
     */
    class Image : public Widget
    {
    public:
        Image();
        ~Image();

        /**
         * @brief Set the image file path.
         */
        void set_path(const std::string &path);
        const std::string &path() const
        {
            return m_path;
        }

        /**
         * @brief Set the image display mode.
         */
        void set_mode(ImageMode mode);
        void set_application_recursive(WaylandWindow *app) override;
        ImageMode mode() const
        {
            return m_mode;
        }

    protected:
        void draw(GraphicsContext &ctx) override;

    private:
        std::string m_path;
        ImageMode m_mode{ImageMode::Normal};
        std::unique_ptr<ImageDriver> m_driver;

        void load_driver();
    };
} // namespace horizon
