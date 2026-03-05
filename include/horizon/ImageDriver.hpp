#pragma once
#include <horizon/GraphicsContext.hpp>
#include <string>

namespace horizon
{
    /**
     * @brief Abstract interface for loading and rendering images.
     */
    class ImageDriver
    {
    public:
        virtual ~ImageDriver() = default;

        /**
         * @brief Load an image from an absolute path.
         * @return true if successful, false otherwise.
         */
        virtual bool load(const std::string &path) = 0;

        /**
         * @brief Draw the image at the specified location and size.
         */
        virtual void draw(GraphicsContext &ctx, int x, int y, int w, int h) = 0;

        /**
         * @brief Get original width of the image.
         */
        virtual int width() const = 0;

        /**
         * @brief Get original height of the image.
         */
        virtual int height() const = 0;
    };
} // namespace horizon
