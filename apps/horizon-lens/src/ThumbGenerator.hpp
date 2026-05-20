#pragma once

#include <horizon/lens/ThumbnailCache.hpp>
#include <string>

namespace horizon::lens
{
    /**
     * @class ThumbGenerator
     * @brief Synchronous thumbnail renderer. Called from ThumbWorker thread only.
     *
     * Generates a PNG thumbnail for images (via stb_image + Cairo), SVGs (via librsvg),
     * and PDFs (via Poppler). Embeds XDG-compatible tEXt metadata chunks into the PNG:
     *
     *   Thumb::URI      — file:// URI of the original file
     *   Thumb::MTime    — unix timestamp (mtime) of the original file at generation time
     *   Thumb::Software — "horizon-lens"
     */
    class ThumbGenerator
    {
    public:
        /**
         * @brief Generates a thumbnail for the given file and saves it to the cache.
         *
         * @param file_path  Absolute path to the source file.
         * @param size       Desired thumbnail size.
         * @return True if the thumbnail was successfully generated and saved.
         */
        static bool generate(const std::string& file_path, ThumbnailSize size = ThumbnailSize::Large);

    private:
        static bool generate_image(const std::string& path, const std::string& out_png,
                                   int width, int height, long long mtime);
        static bool generate_svg(const std::string& path, const std::string& out_png,
                                 int width, int height, long long mtime);
        static bool generate_pdf(const std::string& path, const std::string& out_png,
                                 int width, int height, long long mtime);

        /**
         * @brief Writes XDG-compatible tEXt chunks into a PNG file.
         * Reads the PNG Cairo wrote, adds metadata chunks, rewrites the file.
         */
        static bool embed_png_metadata(const std::string& png_path, const std::string& uri,
                                       long long mtime);

        static std::pair<int,int> dimensions_for_size(ThumbnailSize size);
    };

} // namespace horizon::lens
