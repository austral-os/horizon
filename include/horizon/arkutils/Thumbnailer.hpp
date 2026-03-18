#pragma once

#include <string>

namespace horizon::arkutils
{
    /**
     * @class Thumbnailer
     * @brief Utility class to generate thumbnails for images and PDF files.
     */
    class Thumbnailer
    {
    public:
        /**
         * @brief Generates a thumbnail for a given file and saves it in /tmp.
         * @param path The absolute path to the source file (image or PDF).
         * @param width The desired thumbnail width.
         * @param height The desired thumbnail height.
         * @return The absolute path to the generated thumbnail in /tmp, or an empty string on failure.
         */
        static std::string generate(const std::string &path, int width = 256, int height = 256);

    private:
        static std::string generate_image_thumbnail(const std::string &path, int width, int height);
        static std::string generate_svg_thumbnail(const std::string &path, int width, int height);
        static std::string generate_pdf_thumbnail(const std::string &path, int width, int height);
        
        static std::string get_thumbnail_path(const std::string &source_path, int width, int height);
    };
} // namespace horizon::arkutils
