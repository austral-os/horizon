#include <horizon/arkutils/Thumbnailer.hpp>
#include <horizon/Logger.hpp>
#include <cairo/cairo.h>
#include <librsvg/rsvg.h>
#include <filesystem>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include <horizon/external/stb_image.h>

namespace fs = std::filesystem;

namespace horizon::arkutils
{
    static std::string calculate_hash(const std::string &path)
    {
        // Simple hash based on path and last modified time to identify unique thumbnails
        std::size_t h = std::hash<std::string>{}(path);
        try
        {
            auto ftime = fs::last_write_time(path);
            h ^= std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
        }
        catch (...)
        {
        }
        std::stringstream ss;
        ss << std::hex << h;
        return ss.str();
    }

    std::string Thumbnailer::get_thumbnail_path(const std::string &source_path, int width, int height)
    {
        std::string hash = calculate_hash(source_path);
        std::string thumb_name = "horizon-thumb-" + hash + "-" + std::to_string(width) + "x" + std::to_string(height) + ".png";
        return (fs::temp_directory_path() / thumb_name).string();
    }

    std::string Thumbnailer::generate(const std::string &path, int width, int height)
    {
        if (!fs::exists(path))
            return "";

        std::string thumb_path = get_thumbnail_path(path, width, height);
        if (fs::exists(thumb_path))
            return thumb_path;

        std::string ext = fs::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".svg")
        {
            return generate_svg_thumbnail(path, width, height);
        }
        else if (ext == ".pdf")
        {
            return generate_pdf_thumbnail(path, width, height);
        }
        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif")
        {
            return generate_image_thumbnail(path, width, height);
        }

        return "";
    }

    std::string Thumbnailer::generate_image_thumbnail(const std::string &path, int width, int height)
    {
        int img_w, img_h, channels;
        unsigned char *data = stbi_load(path.c_str(), &img_w, &img_h, &channels, 4);

        if (!data)
            return "";

        // Cairo CAIRO_FORMAT_ARGB32 expectations: BGRA in memory on Little-Endian.
        // STB gives RGBA.
        for (int i = 0; i < img_w * img_h * 4; i += 4)
        {
            std::swap(data[i], data[i + 2]);
        }

        cairo_surface_t *source_s = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, img_w, img_h, img_w * 4);
        cairo_surface_t *dest_s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cairo_t *cr = cairo_create(dest_s);

        double scale_x = (double)width / img_w;
        double scale_y = (double)height / img_h;
        double scale = std::min(scale_x, scale_y);

        // Center and scale
        double tx = (width - img_w * scale) / 2.0;
        double ty = (height - img_h * scale) / 2.0;

        cairo_translate(cr, tx, ty);
        cairo_scale(cr, scale, scale);
        cairo_set_source_surface(cr, source_s, 0, 0);
        cairo_paint(cr);

        std::string thumb_path = get_thumbnail_path(path, width, height);
        cairo_surface_write_to_png(dest_s, thumb_path.c_str());

        cairo_destroy(cr);
        cairo_surface_destroy(dest_s);
        cairo_surface_destroy(source_s);
        stbi_image_free(data);

        return thumb_path;
    }

    std::string Thumbnailer::generate_svg_thumbnail(const std::string &path, int width, int height)
    {
        GError *error = nullptr;
        RsvgHandle *handle = rsvg_handle_new_from_file(path.c_str(), &error);
        if (!handle)
        {
            if (error) g_error_free(error);
            return "";
        }

        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cairo_t *cr = cairo_create(surface);

        RsvgRectangle viewport = {0, 0, (double)width, (double)height};
        rsvg_handle_render_document(handle, cr, &viewport, nullptr);

        std::string thumb_path = get_thumbnail_path(path, width, height);
        cairo_surface_write_to_png(surface, thumb_path.c_str());

        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        g_object_unref(handle);

        return thumb_path;
    }

    std::string Thumbnailer::generate_pdf_thumbnail(const std::string &path, int width, int height)
    {
        std::string thumb_path = get_thumbnail_path(path, width, height);
        // pdftocairo appends extension if we provide a prefix, but we want a specific path.
        // We'll use a temporary prefix and then move the file.
        std::string prefix = thumb_path + "-tmp";
        
        std::stringstream cmd;
        cmd << "pdftocairo -png -singlefile -scale-to-x " << width 
            << " -scale-to-y " << height << " \"" << path << "\" \"" << prefix << "\" > /dev/null 2>&1";
        
        int res = std::system(cmd.str().c_str());
        if (res == 0)
        {
            std::string generated = prefix + ".png";
            if (fs::exists(generated))
            {
                fs::rename(generated, thumb_path);
                return thumb_path;
            }
        }

        return "";
    }
} // namespace horizon::arkutils
